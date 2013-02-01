require "cairo"

local surface = cairo.imagesurface(cairo.format.RGB24, 256, 256)
local context = cairo.context(surface)

-- a custom shape that could be wrapped in a function
local x0      = 25.6   -- parameters like context:rectangle
local y0      = 25.6
local rectwidth  = 204.8
local rectheight = 204.8
local radius = 102.4   -- and an approximate curvature radius

local x1
local y1

x1=x0+rectwidth
y1=y0+rectheight
if rectwidth == 0 or rectheight == 0 then
    return
elseif rectwidth/2<radius then
    if rectheight/2<radius then
        context:moveto(x0,(y0 + y1)/2)
        context:curveto(x0 ,y0, x0, y0,(x0 + x1)/2, y0)
        context:curveto(x1, y0, x1, y0, x1,(y0 + y1)/2)
        context:curveto(x1, y1, x1, y1,(x1 + x0)/2, y1)
        context:curveto(x0, y1, x0, y1, x0,(y0 + y1)/2)
    else
        context:moveto(x0, y0 + radius)
        context:curveto(x0 ,y0, x0, y0,(x0 + x1)/2, y0)
        context:curveto(x1, y0, x1, y0, x1, y0 + radius)
        context:lineto(x1 , y1 - radius)
        context:curveto(x1, y1, x1, y1,(x1 + x0)/2, y1)
        context:curveto(x0, y1, x0, y1, x0, y1- radius)
    end
else
    if rectheight/2<radius then
        context:moveto(x0,(y0 + y1)/2)
        context:curveto(x0 , y0, x0 , y0, x0 + radius, y0)
        context:lineto(x1 - radius, y0)
        context:curveto(x1, y0, x1, y0, x1,(y0 + y1)/2)
        context:curveto(x1, y1, x1, y1, x1 - radius, y1)
        context:lineto(x0 + radius, y1)
        context:curveto(x0, y1, x0, y1, x0,(y0 + y1)/2)
    else
        context:moveto(x0, y0 + radius)
        context:curveto(x0 , y0, x0 , y0, x0 + radius, y0)
        context:lineto(x1 - radius, y0)
        context:curveto(x1, y0, x1, y0, x1, y0 + radius)
        context:lineto(x1 , y1 - radius)
        context:curveto(x1, y1, x1, y1, x1 - radius, y1)
        context:lineto(x0 + radius, y1)
        context:curveto(x0, y1, x0, y1, x0, y1- radius)
    end
end
context:closepath()

context:setsourcergb(0.5, 0.5, 1)
context:fillpreserve()
context:setsourcergba(0.5, 0, 0, 0.5)
context:setlinewidth(10.0)
context:stroke()

surface:writetopng('curverectangle.png')

