/*
** $Id: ltable.h,v 2.21 2015/11/03 15:47:30 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#ifndef ltable_h
#define ltable_h

#include <uvm/lobject.h>


#define gnode(t,i)	(&(t)->node[i])
#define gval(n)		(&(n)->i_val)
#define gnext(n)	((n)->i_key.nk.next)


/* 'const' to avoid wrong writings that can mess up field 'next' */
#define gkey(n)		lua_cast(const TValue*, (&(n)->i_key.tvk))

/*
** writable version of 'gkey'; allows updates to individual fields,
** but not to the whole (which has incompatible type)
*/
#define wgkey(n)		(&(n)->i_key.nk)

#define invalidateTMcache(t)	((t)->flags = 0)


/* returns the key, given the value of a table entry */
#define keyfromval(v) \
  (gkey(lua_cast(Node *, lua_cast(char *, (v)) - offsetof(Node, i_val))))


LUAI_FUNC const TValue *luaH_getint(uvm_types::GcTable *t, lua_Integer key);
LUAI_FUNC void luaH_setint(lua_State *L, uvm_types::GcTable *t, lua_Integer key,
    TValue *value);
LUAI_FUNC const TValue *luaH_getshortstr(uvm_types::GcTable *t, uvm_types::GcString *key);
LUAI_FUNC const TValue *luaH_getstr(uvm_types::GcTable *t, uvm_types::GcString *key);
LUAI_FUNC const TValue *luaH_get(uvm_types::GcTable *t, const TValue *key);
LUAI_FUNC TValue *luaH_newkey(lua_State *L, uvm_types::GcTable *t, const TValue *key, bool allow_lightuserdata = false);
LUAI_FUNC TValue *luaH_set(lua_State *L, uvm_types::GcTable* t, const TValue *key, bool allow_lightuserdata = false);
LUAI_FUNC uvm_types::GcTable *luaH_new(lua_State *L);
LUAI_FUNC void luaH_resize(lua_State *L, uvm_types::GcTable *t, unsigned int nasize,
    unsigned int nhsize);
LUAI_FUNC void luaH_resizearray(lua_State *L, uvm_types::GcTable *t, unsigned int nasize);
LUAI_FUNC void luaH_free(lua_State *L, uvm_types::GcTable *t);
LUAI_FUNC int luaH_next(lua_State *L, uvm_types::GcTable *t, StkId key);
LUAI_FUNC int luaH_getn(uvm_types::GcTable *t);
LUAI_FUNC void luaH_setisonlyread(lua_State *L, uvm_types::GcTable *t, bool isOnlyRead);

#if defined(LUA_DEBUG)
LUAI_FUNC Node *luaH_mainposition(const Table *t, const TValue *key);
LUAI_FUNC int luaH_isdummy(Node *n);
#endif


#endif
