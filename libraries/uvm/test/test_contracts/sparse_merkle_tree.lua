type Storage = {
    depth: int,
    default_nodes: Array<string> -- // array of bytes_string
}

var M = Contract<Storage>()

let hash = sha256_hex

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

let function create_default_nodes(depth: int)
    let default_hash = hash('0000000000000000000000000000000000000000000000000000000000000000')
    let default_nodes = [ default_hash ]
    var level: int = 1
    while level <= depth do
        let prev_default = tostring(default_nodes[level])
        default_nodes[1 + #default_nodes] = hash(prev_default .. prev_default)
        level = level + 1
    end
    return default_nodes
end

let function bytes_to_bigint_as_big_endian(bytes_hex: string)
    let bytes = hex_to_bytes(bytes_hex)
    if not bytes then
        return error("invalid bytes hex")
    end
    var result = safemath.bigint(0)
    var i: int = 1
    var bigint256 = safemath.bigint(256)
    while i <= #bytes do
        let byte = tointeger(bytes[i])
        result = safemath.add(safemath.mul(result, bigint256), safemath.bigint(byte))
        i = i + 1
    end
    return result
end

function M:init()
    self.storage.depth = 64
    self.storage.default_nodes = create_default_nodes(self.storage.depth)
end

offline function M:depth(_: string)
    return self.storage.depth
end

-- check the proof of the leaf as 'uid' is valid
-- TODO: uid use hex format
-- arg format: uid(big int string), leave_hash(hex string), tree_root(hex string), proof(hex string)
-- bytes represented by hex string
function M:verify(arg: string)
    let parsed = parse_at_least_args(arg, 4, "need arg format: uid, leave_hash, tree_root, proof")
    let uid = tostring(parsed[1])
    let leave_hash = tostring(parsed[2])
    let tree_root = tostring(parsed[3])
    let proof = tostring(parsed[4])
    if #proof > 1028 then
        return error("invalid proof size")
    end
    let proofbits_hex = string.sub(proof, 0, 16)
    if #proofbits_hex < 1 then
        return error("invalid proofbits hex")
    end
    var proofbits = bytes_to_bigint_as_big_endian(proofbits_hex)
    var index = safemath.bigint(uid)
    if (index == nil) or (proofbits == nil) then
        return error("invalid uid or proofbits")
    end
    var p = 8  -- tmp position in proof bytes
    var computed_hash = tostring(leave_hash or self.storage.default_nodes[#self.storage.default_nodes])

    let bigint2 = safemath.bigint(2)
    var d: int = 1
    let depth = self.storage.depth
    while d <= depth do
        var proof_element: string
        if safemath.toint(safemath.rem(proofbits, bigint2)) == 0 then
            proof_element = self.storage.default_nodes[d]
        else
            proof_element = string.sub(proof, p*2+1, 2*(p+32))
            p = p + 32
        end
        if safemath.toint(safemath.rem(index, bigint2)) == 0 then
            computed_hash = hash(computed_hash .. proof_element)
        else
            computed_hash = hash(proof_element .. computed_hash)
        end
        proofbits = safemath.div(proofbits, bigint2)
        index = safemath.div(index, bigint2)
        d = d + 1
    end
    let diff = safemath.toint(safemath.sub(bytes_to_bigint_as_big_endian(computed_hash), bytes_to_bigint_as_big_endian(tree_root)))
    let diffIsZero = (diff == 0)
    if not diffIsZero then
        print("smt verify failed")
        print("computed_hash: ", computed_hash)
        print("tree_root: ", tree_root)
        print("diff: ", safemath.toint(safemath.sub(bytes_to_bigint_as_big_endian(computed_hash), bytes_to_bigint_as_big_endian(tree_root))))
    end
    return diffIsZero
end

function M:on_destory()
    error("disabled")
end

return M
