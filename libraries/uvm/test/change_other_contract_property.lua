print('begin change_other_contract_property_contract script')
local contract1 = import_contract 'change_other_contract_property_contract'
local result1 = contract1:start('world')
print("result1: ", result1)
