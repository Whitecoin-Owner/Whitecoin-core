print('begin load_simple_contract script')
local contract1 = import_contract 'simple_contract'
local result1 = contract1:start('world')
print("result1: ", result1)
