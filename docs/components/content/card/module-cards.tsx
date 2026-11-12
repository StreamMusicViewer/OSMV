import {
  Clock,
  Music,
  Activity,
  Video,
  Cpu,
  Timer,
  FileText,
  Tv,
  Layers,
} from 'lucide-react'

import { Card } from '@/components'

export function ModuleCards() {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mt-10">
      <Card
        title="Now Playing"
        icon={<Music className="w-5 h-5" />}
        description="Real-time track detection (WinRT SMTC / D-Bus MPRIS) with OBS Overlay Studio, multi-scene presets, and Snaz exports."
        variant="default"
        href="/docs/playing-now"
      />

      <Card
        title="Time & Date"
        icon={<Clock className="w-5 h-5" />}
        description="Live system clock (Time.txt) and localized date (Date.txt) with customizable format tokens."
        variant="default"
        href="/docs/time"
      />

      <Card
        title="Timers & Chronos"
        icon={<Timer className="w-5 h-5" />}
        description="Target date Countdown, Chrono Down break timer, and Chrono Up stopwatch with interactive Start/Pause/Reset."
        variant="default"
        href="/docs/timers"
      />

      <Card
        title="System Info"
        icon={<Cpu className="w-5 h-5" />}
        description="Lightweight hardware metrics (CPU, RAM, processes, upload/download speeds) written to SystemInfo.txt."
        variant="default"
        href="/docs/system-info"
      />

      <Card
        title="Textline & Dynamic Files"
        icon={<FileText className="w-5 h-5" />}
        description="Rotate custom text lines automatically and fetch remote HTTP/HTTPS text or image files into OBS assets."
        variant="default"
        href="/docs/text-files"
      />

      <Card
        title="Discord RPC"
        icon={<Activity className="w-5 h-5" />}
        description="Native C++ Discord IPC Listening activity with iTunes cover lookup, album metadata, and status badges."
        variant="default"
        href="/docs/discord-rpc"
      />

      <Card
        title="Twitch Helix"
        icon={<Tv className="w-5 h-5" />}
        description="Automatic localhost OAuth, live viewer counts (TwitchViewers.txt), category search, and stream title/game updates."
        variant="default"
        href="/docs/twitch"
      />

      <Card
        title="OBS Integration"
        icon={<Video className="w-5 h-5" />}
        description="Configure OBS Browser, Text, and Image sources using OSMV overlays and generated files."
        variant="default"
        href="/docs/obs"
      />

      <Card
        title="Architecture & RAM"
        icon={<Layers className="w-5 h-5" />}
        description="Explore the C++20 dual-process model (~4.5-6 MB daemon / event-driven Dear ImGui GUI) and local IPC."
        variant="default"
        href="/docs/architecture"
      />
    </div>
  )
}
