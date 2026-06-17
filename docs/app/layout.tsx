import type { Metadata } from 'next'
import { Head } from 'nextra/components'
import { getPageMap } from 'nextra/page-map'
import { Layout, Navbar } from 'nextra-theme-docs'
import type { ReactNode } from 'react'

import { Logo } from '@/components/media'
import 'nextra-theme-docs/style.css'
import './global.css'

export const metadata: Metadata = {
  description: 'Documentation for OSMV - OBS Stream Music Viewer',
  keywords: [
    'OSMV',
    'Documentation',
    'OBS',
    'Stream',
    'Music',
    'Discord RPC',
    'Widget',
    'Streaming',
    'Overlay',
  ],
  title: {
    default: 'OSMV Docs',
    template: '%s | OSMV Docs',
  },
}

const navbar = <Navbar logo={<Logo />} projectLink="https://github.com/StreamMusicViewer/OSMV" />

export default async function RootLayout({ children }: { children: ReactNode }) {
  const pageMap = await getPageMap()

  return (
    <html lang="en" dir="ltr" suppressHydrationWarning>
      <Head backgroundColor={{ dark: '#141d29' }} />
      <body>
        <Layout
          navbar={navbar}
          pageMap={pageMap}
          docsRepositoryBase="https://github.com/StreamMusicViewer/OSMV/tree/main/docs"
          sidebar={{ defaultMenuCollapseLevel: 1 }}
        >
          {children}
        </Layout>
      </body>
    </html>
  )
}
