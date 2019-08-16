/*
** $Id: lcorolib.c,v 1.9 2014/11/02 19:19:04 roberto Exp $
** Coroutine Library
** See Copyright Notice in lua.h
*/

#define lcorolib_cpp

#include "uvm/lprefix.h"


#include <stdlib.h>

#include "uvm/lua.h"

#include "uvm/lauxlib.h"
#include "uvm/lualib.h"


static lua_State *getco(lua_State *L) {
    lua_State *co = lua_tothread(L, 1);
    luaL_argcheck(L, co, 1, "thread expected");
    return co;
}


static int auxresume(lua_State *L, lua_State *co, int narg) {
    int status;
    if (!lua_checkstack(co, narg)) {
        lua_pushliteral(L, "too many arguments to resume");
        return -1;  /* error flag */
    }
    if (lua_status(co) == LUA_OK && lua_gettop(co) == 0) {
        lua_pushliteral(L, "cannot resume dead coroutine");
        return -1;  /* error flag */
    }
    lua_xmove(L, co, narg);
    status = lua_resume(co, L, narg);
    if (status == LUA_OK || status == LUA_YIELD) {
        int nres = lua_gettop(co);
        if (!lua_checkstack(L, nres + 1)) {
            lua_pop(co, nres);  /* remove results anyway */
            lua_pushliteral(L, "too many results to resume");
            return -1;  /* error flag */
        }
        lua_xmove(co, L, nres);  /* move yielded values */
        return nres;
    }
    else {
        lua_xmove(co, L, 1);  /* move error message */
        return -1;  /* error flag */
    }
}


static int luaB_coresume(lua_State *L) {
    lua_State *co = getco(L);
    int r;
    r = auxresume(L, co, lua_gettop(L) - 1);
    if (r < 0) {
        lua_pushboolean(L, 0);
        lua_insert(L, -2);
        return 2;  /* return false + error message */
    }
    else {
        lua_pushboolean(L, 1);
        lua_insert(L, -(r + 1));
        return r + 1;  /* return true + 'resume' returns */
    }
}


static int luaB_auxwrap(lua_State *L) {
    lua_State *co = lua_tothread(L, lua_upvalueindex(1));
    int r = auxresume(L, co, lua_gettop(L));
    if (r < 0) {
        if (lua_isstring(L, -1)) {  /* error object is a string? */
            luaL_where(L, 1);  /* add extra info */
            lua_insert(L, -2);
            lua_concat(L, 2);
        }
        return lua_error(L);  /* propagate error */
    }
    return r;
}

static const luaL_Reg co_funcs[] = {
    { nullptr, nullptr }
};



LUAMOD_API int luaopen_coroutine(lua_State *L) {
    luaL_newlib(L, co_funcs);
    return 1;
}

