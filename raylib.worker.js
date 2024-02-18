import { makeMessagesHandler } from './raylib.js'

const font = new FontFace(
  "grixel",
  "url(fonts/acme_7_wide_xtnd.woff)",
)

self.fonts.add(font);

font.load().catch(console.error)

onmessage = makeMessagesHandler(self)
