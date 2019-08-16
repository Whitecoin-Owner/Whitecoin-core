print("test_math begin")
let a1 = 123
let a2 = 123.456
let a3 = math.abs(-a1)
let a4 = math.tointeger(a2)
let a5 = math.tointeger(nil)
let a6 = 123.45678901234567890
let a7 = math.floor(123.4)
let a8 = math.floor(-123.4)
let a9 = math.max(3, 4, 2)
let a10 = math.min(3, 4, 2)
let a11 = math.type(a1)
let a12 = math.type(a2)
let a13 = math.type('123')

let a14 = math.pi
let a15 = math.maxinteger
let a16 = math.mininteger

let a17 = a1 / 3
let a18 = {value: a17, a1: a1}
let a18_json = json.dumps(a18)
let a19 = json.dumps({value:4 * 1.0})

print('a1=', a1)
print('a2=', a2)
print('a3=', a3)
print('a4=', a4)
print('a5=', a5)
print('a6=', a6)
print('a7=', a7)
print('a8=', a8)
print('a9=', a9)
print('a10=', a10)
print('a11=', a11)
print('a12=', a12)
print('a13=', a13)
print('a14=', a14)
print('a15=', a15)
print('a16=', a16)
print("a17=", a17)
print("a18=", a18)
print("a18_json=", a18_json)
print("a19=", a19)

print("test_math end")
