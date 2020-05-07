x = array int 5 5,2,9,4,8
y = arraylist int 5 1,6,7,4,3
i = 0
i2 = 0
print x
while (i == 5 == 0)
{
    i2 = 0
    while (i2 == 4 == 0)
    {
        left = x[i2]
        right = x[(i2+1)]
        if (left > right) {
            x[i2] = right
            x[i2+1] = left
        }
        i2 = i2 + 1
    }
    i = i + 1
}
print x
print y
i = 0
i2 = 0
while (i == 5 == 0)
{
    i2 = 0
    while (i2 == 4 == 0)
    {
        left = y[i2]
        right = y[(i2+1)]
        if (left > right) {
            y[i2] = right
            y[i2+1] = left
        }
        i2 = i2 + 1
    }
    i = i + 1
}
print y

            
        
    
