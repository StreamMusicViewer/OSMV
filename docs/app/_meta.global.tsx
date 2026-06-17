import { Box, Wand, Info, RectangleEllipsis } from 'lucide-react'
import type { MetaRecord } from 'nextra'

interface MenuIconProps {
  icon: React.ComponentType<{ size?: number }>
  title: string
}

function MenuIcon({ icon: Icon, title }: MenuIconProps) {
  return (
    <span className="flex items-center gap-1.5">
      <Icon size={16} />
      <span>{title}</span>
    </span>
  )
}

const GUIDE: MetaRecord = {
  index: 'Introduction',

  // Modules Section
  '---modules': {
    type: 'separator',
    title: <MenuIcon icon={Box} title="Modules" />,
  },
  'playing-now': {
    title: 'Now Playing',
    items: {
      index: 'Overview',
      'json-data': 'Now Playing JSON',
    },
  },
  time: 'Time Module',
  'discord-rpc': 'Discord RPC',

  // Guides Section
  '---guides': {
    type: 'separator',
    title: <MenuIcon icon={Wand} title="Guides" />,
  },
  obs: 'OBS Integration',
  installation: 'Installation & Compilation',
  architecture: 'Architecture & Performance',

  // FAQ Section
  '---more': {
    type: 'separator',
    title: <MenuIcon icon={RectangleEllipsis} title="More" />,
  },
  faq: 'FAQ',
}

export default {
  index: {
    type: 'page',
    display: 'hidden',
  },
  docs: {
    type: 'page',
    title: 'Documentation',
    items: GUIDE,
  },
  download: {
    type: 'page',
    title: 'Download',
    href: 'https://github.com/StreamMusicViewer/OSMV/releases',
  },
}
