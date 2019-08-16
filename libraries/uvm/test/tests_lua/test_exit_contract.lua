local M = {}

function M:init()

end

function M:start()
	pprint("test exit contract start")
	local a = 1 > 2
	error('hello, exit message here')
	pprint("after exit")
end

pprint("test exit contract loaded")

return M
