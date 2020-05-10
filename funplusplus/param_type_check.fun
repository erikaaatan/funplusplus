x = array int 5 5,2,9,4,8
y = arraylist int 5 1,6,7,4,3
print x
sort = fun (array int arr) {
i = 0
i2 = 0
while (i == 5 == 0)
{
    i2 = 0
    while (i2 == 4 == 0)
    {
        left = arr[i2]
        right = arr[(i2+1)]
        if (left > right) {
            arr[i2] = right
            arr[i2+1] = left
        }
        i2 = i2 + 1
    }
    i = i + 1
}
print arr
}
sort(x)
two = 2
sort(two)
