type Storage = {}

var M = Contract<Storage>()

function M:init()
    print("init api called")
end

function hello()
	a = 123
end

function M:start(arg: string)
    print("start api called")
end

return M
