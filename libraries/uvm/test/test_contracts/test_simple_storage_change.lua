type Storage = {
    num: int
}

var M = Contract<Storage>()

function M:init()
    self.storage.num = 0
end

function M:update(arg: string)
    for i=1,1000,1 do
        self.storage.num = self.storage.num + 1
        let a = tostring(self.storage.num) .. "a"
    end
    let numStr = tostring(self.storage.num)
    emit StorageUpdated(numStr)
end

offline function M:query(arg: string)
    let numStr = tostring(self.storage.num)
    return numStr
end

return M
