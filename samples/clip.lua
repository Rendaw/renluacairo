require "cairo"

local surface = cairo.imagesurface(cairo.format.RGB24, 256, 256)
local context = cairo.context(surface)

context:setsourcergba(1, 1, 1, 1)
context:arc(128.0, 128.0, 76.8, 0, 2 * math.pi)
context:clip(cr)

context:newpath()  -- current path is not
		   -- consumed by context:clip()
context:rectangle(0, 0, 256, 256)
context:fill()
context:setsourcergb(0, 1, 0)
context:moveto(0, 0)
context:lineto(256, 256)
context:moveto(256, 0)
context:lineto(0, 256)
context:setlinewidth(10.0)
context:stroke()

surface:writetopng('clip.png')

