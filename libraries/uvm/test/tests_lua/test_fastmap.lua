print("test_fastmap begin")

var a1 = fast_map_get('balances', 'user1')
print("a1: ", a1)

fast_map_set('balances', 'user1', 123)
var a2 = fast_map_get('balances', 'user1')
print("a2: ", a2)


fast_map_set('balances', 'user1', 234)
var a3 = fast_map_get('balances', 'user1')
print("a3: ", a3)

fast_map_set('balances', 'user2', 'world')
var b1 = fast_map_get('balances', 'user2')
print('b1: ', b1)

fast_map_set('balances', 'user2', nil)
var b3 = fast_map_get('balances', 'user2')
print('b3: ', b3)


fast_map_set('balances', 'user1', 'hello')


var a4 = fast_map_get('balances', 'user1')
print("a4: ", a4)

print("test_fastmap end")
