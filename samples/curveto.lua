require "cairo"

local surface = cairo.imagesurface(cairo.format.RGB24, 256, 256)
local context = cairo.context(surface)

local x=25.6
local y=128.0
local x1=102.4
local y1=230.4
local x2=153.6
local y2=25.6
local x3=230.4
local y3=128.0

context:setsourcergba(1, 1, 1, 1)
context:moveto(x, y)
context:curveto(x1, y1, x2, y2, x3, y3)

context:setlinewidth(10.0)
context:stroke()

context:setsourcergba(1, 0.2, 0.2, 0.6)
context:setlinewidth(6.0)
context:moveto(x,y)   context:lineto(x1,y1)
context:moveto(x2,y2) context:lineto(x3,y3)
context:stroke()

surface:writetopng('curveto.png')

