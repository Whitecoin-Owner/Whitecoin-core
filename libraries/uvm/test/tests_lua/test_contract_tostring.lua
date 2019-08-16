type Storage = {

}

var M = Contract<Storage>()

function M:init()

end

function M:start(_: string)
    let a = 123
    return tostring(a)
end

return M
