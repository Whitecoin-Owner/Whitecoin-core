/*
** $Id: lfunc.h,v 2.15 2015/01/13 15:49:11 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#ifndef lfunc_h
#define lfunc_h


#include "uvm/lobject.h"


#define sizeCclosure(n)	(lua_cast(int, sizeof(CClosure)) + \
                         lua_cast(int, sizeof(TValue)*((n)-1)))

#define sizeLclosure(n)	(lua_cast(int, sizeof(LClosure)) + \
                         lua_cast(int, sizeof(TValue *)*((n)-1)))


/* test whether thread is in 'twups' list */
#define isintwups(L)	(L->twups != L)


/*
** maximum number of upvalues in a closure (both C and Lua). (Value
** must fit in a VM register.)
*/
#define MAXUPVAL	255


/*
** Upvalues for Lua closures
*/
struct UpVal {
    TValue *v;  /* points to stack or to its own value */
    lu_mem refcount;  /* reference counter */
    union {
        struct {  /* (when open) */
            UpVal *next;  /* linked list */
            int touched;  /* mark to avoid cycles with dead threads */
        } open;
        TValue value;  /* the value (when closed) */
    } u;
};

#define upisopen(up)	((up)->v != &(up)->u.value)


LUAI_FUNC uvm_types::GcProto *luaF_newproto(lua_State *L);
LUAI_FUNC uvm_types::GcCClosure *luaF_newCclosure(lua_State *L, int nelems);
LUAI_FUNC uvm_types::GcLClosure *luaF_newLclosure(lua_State *L, int nelems);
LUAI_FUNC void luaF_initupvals(lua_State *L, uvm_types::GcLClosure *cl);
LUAI_FUNC UpVal *luaF_findupval(lua_State *L, StkId level);
LUAI_FUNC void luaF_close(lua_State *L, StkId level);
LUAI_FUNC void luaF_freeproto(lua_State *L, uvm_types::GcProto *f);
LUAI_FUNC const char *luaF_getlocalname(const uvm_types::GcProto *func, int local_number,
    int pc);


#endif
