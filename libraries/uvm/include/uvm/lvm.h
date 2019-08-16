/*
** $Id: lvm.h,v 2.39 2015/09/09 13:44:07 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/

#ifndef lvm_h
#define lvm_h


#include "uvm/ldo.h"
#include "uvm/lobject.h"
#include "uvm/ltm.h"
#include <memory>


#if !defined(LUA_NOCVTN2S)
#define cvt2str(o)	ttisnumber(o)
#else
#define cvt2str(o)	0	/* no conversion from numbers to strings */
#endif


#if !defined(LUA_NOCVTS2N)
#define cvt2num(o)	ttisstring(o)
#else
#define cvt2num(o)	0	/* no conversion from strings to numbers */
#endif


/*
** You can define LUA_FLOORN2I if you want to convert floats to integers
** by flooring them (instead of raising an error if they are not
** integral values)
*/
#if !defined(LUA_FLOORN2I)
#define LUA_FLOORN2I		0
#endif


#define tonumber(o,n) \
	(ttisfloat(o) ? (*(n) = fltvalue(o), 1) : luaV_tonumber_(o,n))

#define tointeger(o,i) \
    (ttisinteger(o) ? (*(i) = ivalue(o), 1) : luaV_tointeger(o,i,LUA_FLOORN2I))

#define intop(op,v1,v2) l_castU2S(l_castS2U(v1) op l_castS2U(v2))

#define luaV_rawequalobj(t1,t2)		luaV_equalobj(nullptr,t1,t2)


/*
** fast track for 'gettable': 1 means 'aux' points to resulted value;
** 0 means 'aux' is metamethod (if 't' is a table) or nullptr. 'f' is
** the raw get function to use.
*/
#define luaV_fastget(L,t,k,aux,f) \
  (!ttistable(t)  \
   ? (aux = nullptr, 0)  /* not a table; 'aux' is nullptr and result is 0 */  \
   : (aux = f(hvalue(t), k),  /* else, do raw access */  \
      !ttisnil(aux) ? 1  /* result not nil? 'aux' has it */  \
      : (aux = fasttm(L, hvalue(t)->metatable, TM_INDEX),  /* get metamethod */\
         aux != nullptr  ? 0  /* has metamethod? must call it */  \
         : (aux = luaO_nilobject, 1))))  /* else, final result is nil */

/*
** standard implementation for 'gettable'
*/
#define luaV_gettable(L,t,k,v) { const TValue *aux; \
  if (luaV_fastget(L,t,k,aux,luaH_get)) { setobj2s(L, v, aux); } \
    else luaV_finishget(L,t,k,v,aux); }


/*
** Fast track for set table. If 't' is a table and 't[k]' is not nil,
** call GC barrier, do a raw 't[k]=v', and return true; otherwise,
** return false with 'slot' equal to nullptr (if 't' is not a table) or
** 'nil'. (This is needed by 'luaV_finishget'.) Note that, if the macro
** returns true, there is no need to 'invalidateTMcache', because the
** call is not creating a new entry.
*/
#define luaV_fastset(L,t,k,slot,f,v) \
  (!ttistable(t) \
   ? (slot = nullptr, 0) \
   : (slot = f(hvalue(t), k), \
     ttisnil(slot) ? 0 \
     : (setobj2t(L, lua_cast(TValue *,slot), v), \
        1)))


#define luaV_settable(L,t,k,v) { const TValue *slot; \
  if (!luaV_fastset(L,t,k,slot,luaH_get,v)) \
    luaV_finishset(L,t,k,v,slot); }

namespace uvm {
	namespace core {
		struct ExecuteContext {
			int64_t* insts_executed_count;
			int64_t *stopped_pointer;
			int64_t insts_limit;
			bool has_insts_limit;
			bool use_last_return;
			CallInfo *ci;
			uvm_types::GcLClosure *cl;
			TValue *k;
			StkId base;
			std::stack<contract_info_stack_entry> using_contract_id_stack;

			void step_out(lua_State *L);
			void step_into(lua_State* L);
			void step_over(lua_State* L);
			// execute to next ci called, return whether has next ci to execute
			bool executeToNextCi(lua_State* L);
			bool executeToNextOp(lua_State* L);
			void enter_newframe(lua_State* L);
			void prepare_newframe(lua_State* L);

			void go_resume(lua_State* L);

			std::map<std::string, TValue> view_localvars(lua_State* L) const;
			std::map<std::string, TValue> view_upvalues(lua_State* L) const;
			TValue view_contract_storage_value(lua_State* L, const char *name, const char* fast_map_key, bool is_fast_map) const;
			uint32_t current_line() const;
			std::vector<std::string> view_call_stack(lua_State* L) const;
		};
	}
}

LUAI_FUNC int luaV_equalobj(lua_State *L, const TValue *t1, const TValue *t2);
LUAI_FUNC int luaV_lessthan(lua_State *L, const TValue *l, const TValue *r);
LUAI_FUNC int luaV_lessequal(lua_State *L, const TValue *l, const TValue *r);
LUAI_FUNC int luaV_tonumber_(const TValue *obj, lua_Number *n);
LUAI_FUNC int luaV_tointeger(const TValue *obj, lua_Integer *p, int mode);
LUAI_FUNC void luaV_finishget(lua_State *L, const TValue *t, TValue *key,
    StkId val, const TValue *tm);
LUAI_FUNC void luaV_finishset(lua_State *L, const TValue *t, TValue *key,
    StkId val, const TValue *oldval);
LUAI_FUNC void luaV_finishOp(lua_State *L);
LUAI_FUNC std::shared_ptr<uvm::core::ExecuteContext> luaV_execute(lua_State *L);
// if not sure, don't use result of get_last_execute_context()'s pointer fields
std::shared_ptr<uvm::core::ExecuteContext> get_last_execute_context();
std::shared_ptr<uvm::core::ExecuteContext> set_last_execute_context(std::shared_ptr<uvm::core::ExecuteContext> p);
LUAI_FUNC void luaV_concat(lua_State *L, int total);
LUAI_FUNC lua_Integer luaV_div(lua_State *L, lua_Integer x, lua_Integer y);
LUAI_FUNC lua_Integer luaV_mod(lua_State *L, lua_Integer x, lua_Integer y);
LUAI_FUNC lua_Integer luaV_shiftl(lua_Integer x, lua_Integer y);
LUAI_FUNC void luaV_objlen(lua_State *L, StkId ra, const TValue *rb);

LUAI_FUNC int luaV_strcmp(const uvm_types::GcString *ls, const uvm_types::GcString *rs);


#endif
