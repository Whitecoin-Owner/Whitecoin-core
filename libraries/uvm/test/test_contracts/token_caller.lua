type Storage = {

}

var M = Contract<Storage>()

function M:init()

end

-- arg format: tokenAddress,toAddress,amount
function M:transfer(arg: string)
    let splited = string.split(arg, ',')
    let tokenAddress = tostring(splited[1])
    let addr = tostring(splited[2])
    let amount = tointeger(splited[3])
    if (not is_valid_contract_address(tokenAddress)) or (not is_valid_address(addr)) or (not amount) or (amount < 1) then
        error("invalid argument")
    end
    let tokenContract = import_contract_from_address(tokenAddress)
    pprint('token contract: ', tokenContract)
    if (not tokenContract) or (not tokenContract.transfer) then
        error("invalid token contract address")
    end
    let transferArg = addr .. ',' .. tostring(amount)
    print("transfer arg: ", transferArg)
    tokenContract:transfer(transferArg)
    emit TransferDone(arg)
end

return M
