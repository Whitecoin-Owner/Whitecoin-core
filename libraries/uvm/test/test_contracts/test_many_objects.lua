type Storage = {

}

var M = Contract<Storage>()

function M:init()

end

let function hi(i: string)
    let a = json.dumps({name: "hi", memo: "simple", i: i})
end

function M:hello(arg: string)

    let loop_count = 10000

    for i=1,loop_count do
        hi(tostring(i))
        let a = {name: "hello", age: i, memo: "helloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworld"}
        let b = json.dumps(a)
    end
    print("done")

end

return M
