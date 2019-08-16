print('test_many_string_operations begin')
for i = 1, 10000000 do
    local s = "hello" .. tostring(i)
end
print('test_many_string_operations done')
