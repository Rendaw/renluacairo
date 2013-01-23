require 'cairo'

matrix = cairo.translatematrix(-1, 3)
x, y = matrix:transformpoint(4, 7)
print(tostring(x))
print(tostring(y))

