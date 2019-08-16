type Storage = {
    name: string
}

var M = Contract<Storage>()

function M:init()
    self.storage.name = 'test_global_apis'
end

function M:start(arg: string)
    let now = get_chain_now()
    let valid = is_valid_address(arg)
    let validContract = is_valid_contract_address("CON" .. arg)
    let systemAssetSymbol = get_system_asset_symbol()
    let blockNum = get_header_block_num()
    let precision = get_system_asset_precision()
    let contractAddr = get_current_contract_address()
    let callFrameStackSize = get_contract_call_frame_stack_size()
    let random = get_chain_random()
    let prevContractAddr = get_prev_call_frame_contract_address()
    let prevContractApiName = get_prev_call_frame_api_name()
    print("now is ", now)
    print(arg, " is valid address? ", valid)
    print(arg, " is valid contract address? ", validContract)
    print("systemAssetSymbol: ", systemAssetSymbol)
    print("blockNum: ", blockNum)
    print("precision: ", precision)
    print("current contract address: ", contractAddr)
    print("callFrameStackSize: ", callFrameStackSize)
    print("random: ", random)
    print("prevContractAddr: ", prevContractAddr)
    print("prevContractApiName: ", prevContractApiName)
end

return M
