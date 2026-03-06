using System;
using System.Numerics;
using System.Threading;
using NAudio.Wave;
using NAudio.CoreAudioApi;

namespace OBS_StreamMusicViewer
{
    /// <summary>
    /// Captures desktop audio via WASAPI Loopback (NAudio) and computes
    /// a 20-band logarithmic spectrum — ported from spectralizer's DSP pipeline.
    /// Thread-safe: GetBands() can be called from any thread.
    /// </summary>
    public class AudioCaptureService : IDisposable
    {
        // ─── Configuration ──────────────────────────────────────────────────────
        private const int    BAR_COUNT    = 20;
        private const double LOW_FREQ     = 50.0;    // Hz  (similar to spectralizer default)
        private const double HIGH_FREQ    = 10000.0; // Hz
        private const double GRAVITY      = 0.88;    // smoothing factor (spectralizer default)
        private const double SCALE_SIZE   = 2.5;     // manual scale
        private const double SCALE_BOOST  = 0.0;

        // ─── State ──────────────────────────────────────────────────────────────
        private WasapiLoopbackCapture _capture;
        private readonly object       _lock       = new object();
        private float[]               _pcmBuffer;   // rolling PCM (mono, float)
        private int                   _writePos;
        private int                   _sampleRate   = 44100;
        private int                   _fftSize      = 2048;

        private double[]              _bands        = new double[BAR_COUNT];
        private double[]              _smoothed     = new double[BAR_COUNT];
        private bool                  _disposed;

        // ─── Public interface ───────────────────────────────────────────────────
        public bool IsRunning => _capture != null;

        public void Start(MMDevice device = null)
        {
            if (_capture != null) return;

            try
            {
                _capture = device != null ? new WasapiLoopbackCapture(device) : new WasapiLoopbackCapture();
                _sampleRate = _capture.WaveFormat.SampleRate;
                _fftSize    = NextPow2(_sampleRate / 30); // ~33ms frame
                _fftSize    = Math.Max(_fftSize, 1024);

                _pcmBuffer  = new float[_fftSize * 4]; // rolling ring buffer
                _writePos   = 0;

                _capture.DataAvailable  += OnDataAvailable;
                _capture.RecordingStopped += (s, e) => { };
                _capture.StartRecording();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("[AudioCaptureService] Start error: " + ex.Message);
                _capture?.Dispose();
                _capture = null;
            }
        }

        public void Stop()
        {
            if (_capture == null) return;
            try
            {
                _capture.StopRecording();
                _capture.DataAvailable -= OnDataAvailable;
            }
            catch { }
            finally
            {
                _capture.Dispose();
                _capture = null;
            }
            // Decay to silence
            lock (_lock)
            {
                Array.Clear(_smoothed, 0, _smoothed.Length);
                Array.Clear(_bands,    0, _bands.Length);
            }
        }

        /// <summary>Returns a snapshot of the 20 normalised band amplitudes (0–1).</summary>
        public float[] GetBands()
        {
            double[] snap;
            lock (_lock) { snap = (double[])_smoothed.Clone(); }

            var result = new float[BAR_COUNT];
            for (int i = 0; i < BAR_COUNT; i++)
                result[i] = (float)Math.Max(0.0, Math.Min(1.0, snap[i]));
            return result;
        }

        // ─── Audio data callback ─────────────────────────────────────────────────
        private void OnDataAvailable(object sender, WaveInEventArgs e)
        {
            if (e.BytesRecorded == 0) return;

            var fmt          = _capture.WaveFormat;
            int bytesPerSamp = fmt.BitsPerSample / 8;
            int channels     = fmt.Channels;
            int totalSamples = e.BytesRecorded / (bytesPerSamp * channels);

            // Mix channels to mono and write into the rolling buffer
            for (int i = 0; i < totalSamples; i++)
            {
                float sum = 0f;
                for (int ch = 0; ch < channels; ch++)
                {
                    int offset = (i * channels + ch) * bytesPerSamp;
                    sum += BitConverter.ToSingle(e.Buffer, offset);
                }
                _pcmBuffer[_writePos % _pcmBuffer.Length] = sum / channels;
                _writePos++;
            }

            // Once we have at least fftSize samples, compute spectrum
            if (_writePos >= _fftSize)
                ComputeSpectrum();
        }

        // ─── FFT + spectrum ──────────────────────────────────────────────────────
        private void ComputeSpectrum()
        {
            // Copy latest fftSize samples (most recent frame)
            var frame = new Complex[_fftSize];
            int start = (_writePos - _fftSize + _pcmBuffer.Length * 100) % _pcmBuffer.Length;
            for (int i = 0; i < _fftSize; i++)
            {
                int idx = (start + i) % _pcmBuffer.Length;
                double sample = _pcmBuffer[idx];
                // Hann window (same as spectralizer uses via FFTW)
                double window = 0.5 * (1.0 - Math.Cos(2.0 * Math.PI * i / (_fftSize - 1)));
                frame[i] = new Complex(sample * window, 0);
            }

            // Cooley-Tukey FFT in-place
            FFT(frame);

            // Compute magnitudes (only positive half)
            int binCount = _fftSize / 2 + 1;
            var mags     = new double[binCount];
            for (int i = 0; i < binCount; i++)
                mags[i] = frame[i].Magnitude;

            // ── Logarithmic cutoff frequencies (ported from spectralizer) ────────
            // recalculate_cutoff_frequencies() equivalent
            var lowCut  = new int[BAR_COUNT + 1];
            var highCut = new int[BAR_COUNT + 1];
            double freqConst = Math.Log10(LOW_FREQ / HIGH_FREQ) / ((1.0 / (BAR_COUNT + 1.0)) - 1.0);

            for (int i = 0; i <= BAR_COUNT; i++)
            {
                double freqConstPerBin = HIGH_FREQ *
                    Math.Pow(10.0, (-freqConst) + (((i + 1.0) / (BAR_COUNT + 1.0)) * freqConst));
                double freq = freqConstPerBin / (_sampleRate / 2.0);
                lowCut[i]   = (int)Math.Floor(freq * _fftSize / 4.0);

                if (i > 0)
                {
                    if (lowCut[i] <= lowCut[i - 1])
                        lowCut[i] = lowCut[i - 1] + 1;
                    highCut[i - 1] = lowCut[i - 1];
                }
            }

            // ── generate_bars() equivalent ───────────────────────────────────────
            var rawBars = new double[BAR_COUNT];
            for (int i = 0; i < BAR_COUNT; i++)
            {
                double freqMag = 0.0;
                int lo = lowCut[i], hi = Math.Min(highCut[i], binCount - 1);
                for (int b = lo; b <= hi; b++)
                    freqMag += mags[b];

                int span = hi - lo + 1;
                rawBars[i] = span > 0 ? freqMag / span : 0.0;

                // High-freq boost (spectralizer: log2(2+i) * 100/N)
                rawBars[i] *= Math.Log(2 + i, 2) * (100.0 / BAR_COUNT);
                rawBars[i]  = Math.Sqrt(rawBars[i]);

                // Manual scale (spectralizer: bar * scale_size + scale_boost)
                rawBars[i]  = rawBars[i] * SCALE_SIZE + SCALE_BOOST;

                // Clamp to [0, 1]
                rawBars[i]  = Math.Max(0.0, Math.Min(1.0, rawBars[i]));
            }

            // ── Gravity smoothing (spectralizer: bar = bar*gravity + new*(1-gravity)) ─
            lock (_lock)
            {
                double grav    = GRAVITY;
                double antigrav = 1.0 - grav;
                for (int i = 0; i < BAR_COUNT; i++)
                    _smoothed[i] = _smoothed[i] * grav + rawBars[i] * antigrav;
            }
        }

        // ─── Cooley-Tukey FFT (iterative, in-place) ─────────────────────────────
        private static void FFT(Complex[] buf)
        {
            int n = buf.Length;
            // Bit-reversal permutation
            for (int i = 1, j = 0; i < n; i++)
            {
                int bit = n >> 1;
                for (; (j & bit) != 0; bit >>= 1)
                    j ^= bit;
                j ^= bit;
                if (i < j) { var tmp = buf[i]; buf[i] = buf[j]; buf[j] = tmp; }
            }
            // Butterfly
            for (int len = 2; len <= n; len <<= 1)
            {
                double ang = -2.0 * Math.PI / len;
                var wlen   = new Complex(Math.Cos(ang), Math.Sin(ang));
                for (int i = 0; i < n; i += len)
                {
                    Complex w = Complex.One;
                    for (int j = 0; j < len / 2; j++)
                    {
                        var u = buf[i + j];
                        var v = buf[i + j + len / 2] * w;
                        buf[i + j]           = u + v;
                        buf[i + j + len / 2] = u - v;
                        w *= wlen;
                    }
                }
            }
        }

        private static int NextPow2(int n)
        {
            int p = 1;
            while (p < n) p <<= 1;
            return p;
        }

        // ─── IDisposable ─────────────────────────────────────────────────────────
        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            Stop();
        }
    }
}
