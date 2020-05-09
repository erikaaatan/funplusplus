x = 1
class test = {
    x = 5
    y = 2
    z = fun x=x+1
}
t = test
print t.x
print t.y
t.z()
print t.x
print x
