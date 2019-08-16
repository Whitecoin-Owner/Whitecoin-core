/*
** $Id: lstring.c,v 2.56 2015/11/23 11:32:51 roberto Exp $
** String table (keeps all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#define lstring_cpp
#define LUA_CORE

#include "uvm/lprefix.h"


#include <string.h>

#include "uvm/lua.h"

#include "uvm/ldebug.h"
#include "uvm/ldo.h"
#include "uvm/lmem.h"
#include "uvm/lobject.h"
#include "uvm/lstate.h"
#include "uvm/lstring.h"


#define MEMERRMSG       "not enough memory"

/*
** Lua will use at most ~(2^LUAI_HASHLIMIT) bytes from a string to
** compute its hash
*/
#if !defined(LUAI_HASHLIMIT)
#define LUAI_HASHLIMIT		5
#endif

/*
** equality for long strings
*/
int luaS_eqlngstr(uvm_types::GcString *a, uvm_types::GcString *b) {
	size_t len = a->value.size();
    lua_assert(a->tt == LUA_TLNGSTR && b->tt == LUA_TLNGSTR);
    return (a == b) ||  /* same instance or... */
        ((len == b->value.size()) &&  /* equal length and ... */
        (memcmp(getstr(a), getstr(b), len) == 0));  /* equal contents */
}


unsigned int luaS_hash(const char *str, size_t l, unsigned int seed) {
    unsigned int h = seed ^ lua_cast(unsigned int, l);
    size_t step = (l >> LUAI_HASHLIMIT) + 1;
    for (; l >= step; l -= step)
        h ^= ((h << 5) + (h >> 2) + cast_byte(str[l - 1]));
    return h;
}


unsigned int luaS_hashlongstr(uvm_types::GcString *ts) {
    lua_assert(ts->tt == LUA_TLNGSTR);
    auto h = luaS_hash(getstr(ts), ts->value.size(), 1);
    return h;
}


/*
** Clear API string cache. (Entries cannot be empty, so fill them with
** a non-collectable string.)
*/
void luaS_clearcache(lua_State *L) {
    
}


/*
** Initialize the string table and the string cache
*/
void luaS_init(lua_State *L) {
    int i, j;
    // luaS_resize(L, MINSTRTABSIZE);  /* initial size of string table */
    /* pre-create memory-error message */
    L->memerrmsg = luaS_newliteral(L, MEMERRMSG);
    for (i = 0; i < STRCACHE_N; i++)  /* fill cache with valid strings */
        for (j = 0; j < STRCACHE_M; j++)
            L->strcache[i][j] = L->memerrmsg;
}



/*
** creates a new string object
*/
static uvm_types::GcString *createstrobj(lua_State *L, size_t l, int tag, unsigned int h) {
	uvm_types::GcString *ts;
    auto o =  L->gc_state->gc_new_object<uvm_types::GcString>();
	o->value.resize(l);
	ts = o;
    return ts;
}


uvm_types::GcString *luaS_createlngstrobj(lua_State *L, size_t l) {
	uvm_types::GcString *ts = createstrobj(L, l, LUA_TLNGSTR, L->seed);
	ts->value.resize(l);
    return ts;
}

void luaS_remove(lua_State *L, uvm_types::GcString *ts) {
}


/*
** checks whether short string exists and reuses it or creates a new one
*/
static uvm_types::GcString *internshrstr(lua_State *L, const char *str, size_t l) {
	uvm_types::GcString *ts;
    unsigned int h = luaS_hash(str, l, L->seed);
    lua_assert(str != nullptr);  /* otherwise 'memcmp'/'memcpy' are undefined */
    ts = createstrobj(L, l, LUA_TSHRSTR, h);
    memcpy(getstr(ts), str, l * sizeof(char));
    return ts;
}


/*
** new string (with explicit length)
*/
uvm_types::GcString *luaS_newlstr(lua_State *L, const char *str, size_t l) {
	
	/*if (l <= LUAI_MAXSHORTLEN)  //short string?
        return internshrstr(L, str, l);
    else {
		uvm_types::GcString *ts;
        if (l >= (UVM_MAX_SIZE - sizeof(uvm_types::GcString)) / sizeof(char))
            luaM_toobig(L);
        ts = luaS_createlngstrobj(L, l);
        memcpy(getstr(ts), str, l * sizeof(char));
        return ts;
    }*/
	
	
	auto ts = L->gc_state->gc_intern_string<uvm_types::GcString>(str,l);
	//if (isNewStr) {
	//	ts->value = std::string(str, l);
	//}
	return ts;

}


/*
** Create or reuse a zero-terminated string, first checking in the
** cache (using the string address as a key). The cache can contain
** only zero-terminated strings, so it is safe to use 'strcmp' to
** check hits.
*/
uvm_types::GcString *luaS_new(lua_State *L, const char *str) {
    auto s = luaS_newlstr(L, str, strlen(str));
	return s;
}


uvm_types::GcUserdata *luaS_newudata(lua_State *L, size_t s) {
    if (s > UVM_MAX_SIZE - sizeof(uvm_types::GcUserdata))
        luaM_toobig(L);
	auto o = L->gc_state->gc_new_object<uvm_types::GcUserdata>();
	o->gc_value = L->gc_state->gc_malloc(s);
	o->len = s;
    setuservalue(L, o, luaO_nilobject);
    return o;
}

