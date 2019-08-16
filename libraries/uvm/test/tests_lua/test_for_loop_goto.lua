e = {name="uvm", age=25}
e["123"] = "test"
s = ''
for k in pairs(e) do
	s = s .. ';' .. k .. ':' .. tostring(e[k])
end

print(s)
