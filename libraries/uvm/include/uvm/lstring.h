/*
** $Id: lstring.h,v 1.61 2015/11/03 15:36:01 roberto Exp $
** String table (keep all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#ifndef lstring_h
#define lstring_h

#include "uvm/lgc.h"
#include "uvm/lobject.h"
#include "uvm/lstate.h"


#define sizeludata(l)	(sizeof(union UUdata) + (l))
#define sizeudata(u)	sizeludata((u)->len)

#define luaS_newliteral(L, s)	(luaS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))


/*
** test whether a string is a reserved word
*/
//#define isreserved(s)	((s)->tt == LUA_TSHRSTR && (s)->extra > 0)
#define isreserved(s)	((s)->tt == LUA_TLNGSTR && (s)->extra > 0)


/*
** equality for short strings, which are always internalized
*/
#define eqshrstr(a,b)	check_exp((a)->tt == LUA_TSHRSTR, (a) == (b))


LUAI_FUNC unsigned int luaS_hash(const char *str, size_t l, unsigned int seed);
LUAI_FUNC unsigned int luaS_hashlongstr(uvm_types::GcString *ts);
LUAI_FUNC int luaS_eqlngstr(uvm_types::GcString *a, uvm_types::GcString *b);
LUAI_FUNC void luaS_clearcache(lua_State *L);
LUAI_FUNC void luaS_init(lua_State *L);
LUAI_FUNC void luaS_remove(lua_State *L, uvm_types::GcString *ts);
LUAI_FUNC uvm_types::GcUserdata *luaS_newudata(lua_State *L, size_t s);
LUAI_FUNC uvm_types::GcString *luaS_newlstr(lua_State *L, const char *str, size_t l);
LUAI_FUNC uvm_types::GcString *luaS_new(lua_State *L, const char *str);
LUAI_FUNC uvm_types::GcString *luaS_createlngstrobj(lua_State *L, size_t l);


#endif
