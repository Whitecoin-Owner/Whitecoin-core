print("test cbor encode/decode")

var a1 = [123, "bar", 321, 321, "foo", true, false, nil, [123], [], {"hello": "world", "age": 18}]
var a2 = 123
var a3 = "hello"
var a4 = true
var a5 = 100000000000
var a6 = nil
var a7 = []
var encoded1 = cbor_encode(a1)
var encoded2 = cbor_encode(a2)
var encoded3 = cbor_encode(a3)
var encoded4 = cbor_encode(a4)
var encoded5 = cbor_encode(a5)
var encoded6 = cbor_encode(a6)
var encoded7 = cbor_encode(a7)
print("encoded1: ", encoded1)
print("encoded2: ", encoded2)
print("encoded3: ", encoded3)
print("encoded4: ", encoded4)
print("encoded5: ", encoded5)
print("encoded6: ", encoded6)
print("encoded7: ", encoded7)

var decoded1 = cbor_decode(encoded1)
var decoded2 = cbor_decode(encoded2)
var decoded3 = cbor_decode(encoded3)
var decoded4 = cbor_decode(encoded4)
var decoded5 = cbor_decode(encoded5)
var decoded6 = cbor_decode(encoded6)
var decoded7 = cbor_decode(encoded7)

pprint(a1, " = ", decoded1)
pprint(a2, " = ", decoded2)
pprint(a3, " = ", decoded3)
pprint(a4, " = ", decoded4)
pprint(a5, " = ", decoded5)
pprint(a6, " = ", decoded6)
pprint(a7, " = ", decoded7)
