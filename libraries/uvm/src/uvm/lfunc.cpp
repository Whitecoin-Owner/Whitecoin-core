/*
** $Id: lfunc.c,v 2.45 2014/11/02 19:19:04 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#define lfunc_cpp
#define LUA_CORE

#include "uvm/lprefix.h"


#include <stddef.h>

#include "uvm/lua.h"

#include "uvm/lfunc.h"
#include "uvm/lgc.h"
#include "uvm/lmem.h"
#include "uvm/lobject.h"
#include "uvm/lstate.h"



uvm_types::GcCClosure *luaF_newCclosure(lua_State *L, int n) {
	uvm_types::GcCClosure* o = L->gc_state->gc_new_object<uvm_types::GcCClosure>();
	o->nupvalues = n;
	o->upvalue.resize(n);
    return o;
}


uvm_types::GcLClosure *luaF_newLclosure(lua_State *L, int n) {
	auto o = L->gc_state->gc_new_object<uvm_types::GcLClosure>();
	o->nupvalues = n;
	o->upvals.resize(n);
	uvm_types::GcLClosure *c = gco2lcl(o);
    c->p = nullptr;
    c->nupvalues = cast_byte(n);
    while (n--) c->upvals[n] = nullptr;
    return c;
}

/*
** fill a closure with new closed upvalues
*/
void luaF_initupvals(lua_State *L, uvm_types::GcLClosure *cl) {
    int i;
    for (i = 0; i < cl->nupvalues; i++) {
		UpVal *uv = static_cast<UpVal*>(L->gc_state->gc_malloc(sizeof(UpVal)));
        uv->refcount = 1;
        uv->v = &uv->u.value;  /* make it closed */
        setnilvalue(uv->v);
        cl->upvals[i] = uv;
    }
}


UpVal *luaF_findupval(lua_State *L, StkId level) {
    UpVal **pp = &L->openupval;
    UpVal *p;
    UpVal *uv;
    lua_assert(isintwups(L) || L->openupval == nullptr);
    while (*pp != nullptr && (p = *pp)->v >= level) {
        lua_assert(upisopen(p));
        if (p->v == level)  /* found a corresponding upvalue? */
            return p;  /* return it */
        pp = &p->u.open.next;
    }
    /* not found: create a new upvalue */
	uv = static_cast<UpVal*>(L->gc_state->gc_malloc(sizeof(UpVal)));
    uv->refcount = 0;
    uv->u.open.next = *pp;  /* link it to list of open upvalues */
    uv->u.open.touched = 1;
    *pp = uv;
    uv->v = level;  /* current value lives in the stack */
    if (!isintwups(L)) {  /* thread not in list of threads with upvalues? */
		if (L->twups)
			L->twups->twups = L;
		L->twups = L;
    }
    return uv;
}


void luaF_close(lua_State *L, StkId level) {
    UpVal *uv;
    while (L->openupval != nullptr && (uv = L->openupval)->v >= level) {
        lua_assert(upisopen(uv));
        L->openupval = uv->u.open.next;  /* remove from 'open' list */
		if (uv->refcount == 0)  /* no references? */
			L->gc_state->gc_free(uv);
        else {
            setobj(L, &uv->u.value, uv->v);  /* move value to upvalue slot */
            uv->v = &uv->u.value;  /* now current value lives here */
            luaC_upvalbarrier(L, uv);
        }
    }
}


uvm_types::GcProto *luaF_newproto(lua_State *L) {
	auto f = L->gc_state->gc_new_object<uvm_types::GcProto>();
    return f;
}


void luaF_freeproto(lua_State *L, uvm_types::GcProto *f) {
}


/*
** Look for n-th local variable at line 'line' in function 'func'.
** Returns nullptr if not found.
*/
const char *luaF_getlocalname(const uvm_types::GcProto *f, int local_number, int pc) {
    int i;
    for (i = 0; i < f->locvars.size() && f->locvars[i].startpc <= pc; i++) {
        if (pc < f->locvars[i].endpc) {  /* is variable active? */
            local_number--;
            if (local_number == 0)
                return getstr(f->locvars[i].varname);
        }
    }
    return nullptr;  /* not found */
}

