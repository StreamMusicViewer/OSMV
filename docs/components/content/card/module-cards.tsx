import {
  Clock,
  Music,
  Activity,
  Video,
  Cpu,
} from 'lucide-react'

import { Card } from '@/components'

export function ModuleCards() {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mt-10">
      <Card
        title="Now Playing"
        icon={<Music className="w-5 h-5" />}
        description="Display currently playing music from Spotify, Apple Music, browsers, VLC, and more, complete with album art."
        variant="default"
        href="/docs/playing-now"
      />

      <Card
        title="Time Module"
        icon={<Clock className="w-5 h-5" />}
        description="Write current system time to a text file with customizable formatting ($h, $m, $s, $tt) for overlays."
        variant="default"
        href="/docs/time"
      />

      <Card
        title="Discord RPC"
        icon={<Activity className="w-5 h-5" />}
        description="Showcase your active media or custom status directly on your Discord profile with automated artwork lookup."
        variant="default"
        href="/docs/discord-rpc"
      />

      <Card
        title="OBS Integration"
        icon={<Video className="w-5 h-5" />}
        description="Add a beautiful glassmorphism widget to OBS Studio using the provided local HTML/CSS template."
        variant="default"
        href="/docs/obs"
      />

      <Card
        title="Architecture & RAM"
        icon={<Cpu className="w-5 h-5" />}
        description="Understand the dual-process architecture (15MB Daemon / 378MB GUI) and its performance advantages."
        variant="default"
        href="/docs/architecture"
      />
    </div>
  )
}
