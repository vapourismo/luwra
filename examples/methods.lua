-- Instantiate type
local p = Point(13, 37)
print("p =", p)

-- Invoke 'scale' method
p:scale(2)
print("p =", p)

-- Access 'x' and 'y' property
print("p.x =", p:x())
print("p.y =", p:y())

-- Modify 'x' property
p:x(10)
print("p.x =", p:x())
