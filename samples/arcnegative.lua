require "cairo"

local surface = cairo.imagesurface(cairo.format.RGB24, 256, 256)
local context = cairo.context(surface)

local xc = 128.0
local yc = 128.0
local radius = 100.0
local angle1 = 45.0  * (math.pi/180.0)  -- angles are specified
local angle2 = 180.0 * (math.pi/180.0)  -- in radians

context:setsourcergba(1, 1, 1, 1)
context:setlinewidth(10.0)
context:arcnegative(xc, yc, radius, angle1, angle2)
context:stroke()

-- draw helping lines
context:setsourcergba(1, 0.2, 0.2, 0.6)
context:setlinewidth(6.0)

context:arc(xc, yc, 10.0, 0, 2*math.pi)
context:fill()

context:arc(xc, yc, radius, angle1, angle1)
context:lineto(xc, yc)
context:arc(xc, yc, radius, angle2, angle2)
context:lineto(xc, yc)
context:stroke()

surface:writetopng('arcnegative.png')

