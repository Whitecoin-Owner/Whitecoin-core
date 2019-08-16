type Storage = {
    id: number
}

var M = Contract<Storage>()

function M:init()
    self.storage.id = 1
end

return M
