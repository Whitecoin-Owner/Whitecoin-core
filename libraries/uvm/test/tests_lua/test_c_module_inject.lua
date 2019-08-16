print("Hi, this is test c module inject caes\n")

local hello = require 'hello'

hello.say_hello('uvm')

print(hello.test_error())

print("after call test_error()")
