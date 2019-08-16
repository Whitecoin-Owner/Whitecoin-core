type Storage = {
    proxyAddr: string,
    data: string
}

var M = Contract<Storage>()

function M:init()
    self.storage.proxyAddr = ''
    self.storage.data = '' -- delegate_call caller's contract must define storages used in callee
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

function M:set_proxy(target: string)
    self.storage.proxyAddr = target
end

function M:set_data(arg: string)
    let res = delegate_call(self.storage.proxyAddr, 'set_data', arg)
    return res
end

function M:pass_set_data1(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let res = delegate_call(parsed[1], 'set_data', parsed[2])
    return res
end

function M:pass_set_data2(arg: string)
    let parsed = parse_at_least_args(arg, 3, "need format: proxyAddr1,proxyAddr2,argument")
    let proxyAddr1 = tostring(parsed[1])
    let proxyAddr2 = tostring(parsed[2])
    let other = tostring(parsed[3])
    let res = delegate_call(proxyAddr1, 'pass_set_data1', proxyAddr2 .. ',' .. other)
    return res
end

function M:call_set_data(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let contract = import_contract_from_address(tostring(parsed[1]))
    let res = contract:set_data(tostring(parsed[2]))
    return res
end

function M:pass_call_data2(arg: string)
    let parsed = parse_at_least_args(arg, 3, "need format: proxyAddr1,proxyAddr2,argument")
    let proxyAddr1 = tostring(parsed[1])
    let proxyAddr2 = tostring(parsed[2])
    let other = tostring(parsed[3])
    let res = delegate_call(proxyAddr1, 'call_set_data', proxyAddr2 .. ',' .. other)
    return res
end

offline function M:hello(name: string)
    let res = delegate_call(self.storage.proxyAddr, 'hello', name)
    print("proxy target response", res)
    return res
end

function M:pass_hello1(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let res = delegate_call(parsed[1], 'hello', parsed[2])
    return res
end

function M:pass_hello2(arg: string)
    let parsed = parse_at_least_args(arg, 3, "need format: proxyAddr1,proxyAddr2,argument")
    let proxyAddr2 = tostring(parsed[2])
    let other = tostring(parsed[3])
    let res = delegate_call(parsed[1], 'pass_hello1', proxyAddr2 .. ',' .. other)
    return res
end

function M:call_hello(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let contract = import_contract_from_address(tostring(parsed[1]))
    let res = contract:hello(tostring(parsed[2]))
    return res
end

function M:on_deposit_asset(arg: string)
    delegate_call(self.storage.proxyAddr, 'on_deposit_asset_by_call', arg)
end

function M:on_deposit_asset_by_call(arg: string)
    return delegate_call(self.storage.proxyAddr, 'on_deposit_asset_by_call', arg)
end

function M:pass_on_deposit_asset_by_call(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let res = delegate_call(parsed[1], 'on_deposit_asset_by_call', parsed[2])
    return res
end

function M:withdraw(arg: string)
    return delegate_call(self.storage.proxyAddr, 'withdraw', arg)
end

function M:pass_withdraw(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let res = delegate_call(parsed[1], 'withdraw', parsed[2])
    return res
end

function M:call_withdraw(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let contract = import_contract_from_address(tostring(parsed[1]))
    let res = contract:withdraw(tostring(parsed[2]))
    return res
end

function M:query_balance(arg: string)
    return delegate_call(self.storage.proxyAddr, 'query_balance', arg)
end

function M:pass_query_balance(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let res = delegate_call(parsed[1], 'query_balance', parsed[2])
    return res
end

function M:call_query_balance(arg: string)
    let parsed = parse_at_least_args(arg, 2, "need format: proxyAddr,argument")
    let contract = import_contract_from_address(tostring(parsed[1]))
    let res = contract:query_balance(tostring(parsed[2]))
    return res
end

return M
