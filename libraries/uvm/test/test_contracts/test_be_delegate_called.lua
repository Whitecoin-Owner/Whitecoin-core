type Storage = {
    admin: string,
    data: string
}

var M = Contract<Storage>()

function M:init()
    self.storage.admin = caller_address
    self.storage.data = ''
end


let function get_from_address()
    var from_address: string
    let prev_contract_id = get_prev_call_frame_contract_address()
    if prev_contract_id and is_valid_contract_address(prev_contract_id) then
        from_address = prev_contract_id
    else
        from_address = caller_address
    end
    return from_address
end


-- parse a,b,c format string to [a,b,c]
let function parse_args(arg: string, count: int, error_msg: string)
    if not arg then
        return error(error_msg)
    end
    let parsed = string.split(arg, ',')
    if (not parsed) or (#parsed ~= count) then
        return error(error_msg)
    end
    return parsed
end

let function parse_at_least_args(arg: string, count: int, error_msg: string)
    if not arg then
        return error(error_msg)
    end
    let parsed = string.split(arg, ',')
    if (not parsed) or (#parsed < count) then
        return error(error_msg)
    end
    return parsed
end

let function arrayContains(col: Array<object>, item: object)
    if not item then
        return false
    end
    var value: object
    for _, value in ipairs(col) do
        if value == item then
            return true
        end
    end
    return false
end

function M:set_admin(arg: string)
    let from = get_from_address()
    print("from: " .. from)
    --if self.storage.admin ~= from then
    --    return error("only admin can change admin")
    --end
    if not is_valid_address(arg) then
        return error("invalid admin address")
    end
    self.storage.admin = arg
end

function M:query_admin(_: string)
    return self.storage.admin
end

function M:set_data(arg: string)
    let from = get_from_address()
    --if self.storage.admin ~= from then
    --    return error("only admin can call this api")
    --end
    self.storage.data = arg
end

offline function M:hello(name: string)
    return "hello, name is " .. name .. " and data is " .. (self.storage.data)
end

function M:on_deposit_asset_by_call(arg: string)
    let from = get_from_address()
    --if self.storage.admin ~= from then
    --    return error("only admin can call this api")
    --end
    print("called by proxy with arg " .. arg)
end

function M:withdraw(arg: string)
    let from = get_from_address()
    --if self.storage.admin ~= from then
    --    return error("only admin can call this api")
    --end
    let symbol = get_system_asset_symbol()
    let amount = tointeger(arg)
    let res = transfer_from_contract_to_address(caller_address, symbol, amount)
    if res ~= 0 then
        return error("error when transfer ")
    end
    return res
end

function M:query_balance(arg: string)
    let symbol = get_system_asset_symbol()
    return get_contract_balance_amount(get_current_contract_address(), symbol)
end

return M
