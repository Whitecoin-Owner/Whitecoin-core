print("test_crypto_primitives begin")

var b = {'1FNfecY7xnKmMQyUpc7hbgSHqD2pD8YDJ7': 20000}
print("b tojson is ", tojsonstring(b))
print("sha256(\"\")=", sha256_hex(""))
print("sha3(\"\")=", sha3_hex(""))
print("sha1(\"\")=", sha1_hex(""))
print("ripemd160(\"\")=", ripemd160_hex(""))
print("sha256(\"a123\")=", sha256_hex("a123"))
print("sha3(\"a123\")=", sha3_hex("a123"))
print("sha1(\"a123\")=", sha1_hex("a123"))
print("ripemd160(\"a123\")=", ripemd160_hex("a123"))
var a = hex_to_bytes('a123')
pprint("hex_to_bytes(\"a123\")=", a)
var c = bytes_to_hex(a)
pprint("a123=", c)

print("test_crypto_primitives end")
