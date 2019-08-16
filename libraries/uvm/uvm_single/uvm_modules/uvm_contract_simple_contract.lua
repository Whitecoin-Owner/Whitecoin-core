type Storage = {
    name: string
}

var M = Contract<Storage>()

function M:init()
    self.storage.name = 'simple_contract'
    print('simple contract inited')
end

function M:start(arg: string)
    print("start api called of simple contract")
    print("storage.name=", self.storage.name)
    return "hello " .. arg
end

return M
