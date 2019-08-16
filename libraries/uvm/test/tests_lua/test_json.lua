print("test_json begin")

var a = [1, 2, 3, {name='hi'}]
let a1 = json.dumps(a)
let a2 = json.loads(a1)
let a3 = json.dumps(123)
let a4 = json.loads(a3)
if a4 ~= 123 then
    print("a4=", a4)
    error("error when json dumps/loads 123")
end
let a5 = json.dumps({name: "hello"})
let a6 = json.loads(a5)

pprint('a1=', a1)
pprint('a2=', a2)
pprint('a3=', a3)
pprint('a4=', a4)
pprint('a5=', a5)
pprint('a6=', a6)

print("test_json end")
