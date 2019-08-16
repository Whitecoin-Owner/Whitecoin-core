print("test_time begin")

let a1 = 1234567890
let a2 = time.add(a1, 'second', 3600*24)
let a3 = time.difftime(a1, a2)
let a4 = time.tostr(a1)
let a5 = time.tostr(a2)
print("a1=", a1)
print("a2=", a2)
print("a3=", a3)
print("a4=", a4)
print("a5=", a5)

print("test_time end")
