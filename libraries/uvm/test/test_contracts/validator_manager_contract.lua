type Storage = {
    validators: Map<bool>,
    owner: string
}

var M = Contract<Storage>()

function M:init()
    self.storage.owner = caller_address
    self.storage.validators = {}
    self.storage.validators[caller_address] = true
end

offline function M:checkValidator(address: string)
    if address == self.storage.owner then
        return true
    end
    let validators = self.storage.validators
    if validators[address] then
        return true
    else
        return false
    end
end

offline function M:owner(_: string)
    return self.storage.owner
end

offline function M:validators(_: string)
    let validators = self.storage.validators
    let validatorsStr = json.dumps(validators)
    return validatorsStr
end

function M:toggleValidator(address: string)
    if caller_address ~= self.storage.owner then
        return error("only owner call call this api")
    end
    if not is_valid_address(address) then
        return error("invalid address format")
    end
    self.storage.validators[address] = not (self.storage.validators[address])
end

function M:on_destroy()
    error("destory is disabled")
end

return M
