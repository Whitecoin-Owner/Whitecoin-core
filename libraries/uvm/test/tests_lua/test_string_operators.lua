print('test_string_operators begin')

let a1 = "abc"
let a2 = "def"
let a3 = a1 .. a2 .. "ghj"
let a4 = #a3
let a5 = string.char(75)
let a6 = string.len(a3)

print('a1=', a1)
print('a2=', a2)
print('a3=', a3)
print('a4=', a4)
print('a5=', a5)
print('a6=', a6)

print('test_string_operators end')
