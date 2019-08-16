/*
** $Id: lstate.h,v 2.128 2015/11/13 12:16:51 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/

#ifndef lstate_h
#define lstate_h

#include <string.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <cstddef>
#include <unordered_map>

#include "uvm/lua.h"

#include "uvm/lobject.h"
#include "uvm/ltm.h"
#include "uvm/lzio.h"
#include <uvm/uvm_api.h>
#include <vmgc/vmgc.h>
#include "uvm/lopcodes.h"

#define LUA_MALLOC_TOTAL_SIZE	(500*1024*1024)

#define LUA_COMPILE_ERROR_MAX_LENGTH 4096

#define LUA_API_INTERNAL_ERROR   -1
/*

** Some notes about garbage-collected objects: All objects in Lua must
** be kept somehow accessible until being freed, so all objects always
** belong to one (and only one) of these lists, using field 'next' of
** the 'CommonHeader' for the link:
**
** 'allgc': all objects not marked for finalization;
** 'finobj': all objects marked for finalization;
** 'tobefnz': all objects ready to be finalized;
** 'fixedgc': all objects that are not to be collected (currently
** only small strings, such as reserved words).

*/


struct lua_longjmp;  /* defined in ldo.c */



/* extra stack space to handle TM calls and some other extras */
#define EXTRA_STACK   5


#define BASIC_STACK_SIZE        (2*LUA_MINSTACK)


/* kinds of Garbage Collection */
#define KGC_NORMAL	0
#define KGC_EMERGENCY	1	/* gc was forced by an allocation failure */


/*
** Information about a call.
** When a thread yields, 'func' is adjusted to pretend that the
** top function has only the yielded values in its stack; in that
** case, the actual 'func' value is saved in field 'extra'.
** When a function calls another with a continuation, 'extra' keeps
** the function index so that, in case of errors, the continuation
** function can be called with the correct top.
*/
typedef struct CallInfo {
    StkId func;  /* function index in the stack */
    StkId	top;  /* top for this function */
    struct CallInfo *previous, *next;  /* dynamic call link */
    union {
        struct {  /* only for Lua functions */
            StkId base;  /* base for this function */
            const Instruction *savedpc;
        } l;
        struct {  /* only for C functions */
            lua_KFunction k;  /* continuation in case of yields */
            ptrdiff_t old_errfunc;
            lua_KContext ctx;  /* context info. in case of yields */
        } c;
    } u;
    ptrdiff_t extra;
    short nresults;  /* expected number of results from this function */
    lu_byte callstatus;
} CallInfo;


/*
** Bits in CallInfo status
*/
#define CIST_OAH	(1<<0)	/* original value of 'allowhook' */
#define CIST_LUA	(1<<1)	/* call is running a Lua function */
#define CIST_HOOKED	(1<<2)	/* call is running a debug hook */
#define CIST_FRESH	(1<<3)	/* call is running on a fresh invocation
of luaV_execute */
#define CIST_YPCALL	(1<<4)	/* call is a yieldable protected call */
#define CIST_TAIL	(1<<5)	/* call was tail called */
#define CIST_HOOKYIELD	(1<<6)	/* last hook called yielded */
#define CIST_LEQ	(1<<7)  /* using __lt for __le */

#define isLua(ci)	((ci)->callstatus & CIST_LUA)

/* assume that CIST_OAH has offset 0 and that 'v' is strictly 0/1 */
#define setoah(st,v)	((st) = ((st) & ~CIST_OAH) | (v))
#define getoah(st)	((st) & CIST_OAH)

typedef struct UvmStatePreProcessorFunction
{
    std::list<void*> args;
    void(*processor)(lua_State *L, std::list<void*>*args_list);
    void operator ()(lua_State *L)
    {
        if (nullptr != processor)
            processor(L, &args);
    }
} UvmStatePreProcessorFunction;

inline UvmStatePreProcessorFunction make_lua_state_preprocessor(std::list<void*> &args, void(*processor)(lua_State *L, std::list<void*>*args_list))
{
    UvmStatePreProcessorFunction func;
    func.args = args;
    func.processor = processor;
    return func;
}


// typedef void(*LuaStatePreProcessor)(lua_State *L, void *ptr);

struct lua_State;

namespace uvm_types {
	struct GcString;
	struct GcTable;
}

typedef enum lua_VMState {
	LVM_STATE_NONE = 0,

	LVM_STATE_HALT = 1 << 0,
	LVM_STATE_FAULT = 1 << 1,
	LVM_STATE_BREAK = 1 << 2,
	LVM_STATE_SUSPEND = 1 << 3
} lua_VMState;

struct contract_info_stack_entry {
	std::string contract_id;
	std::string storage_contract_id; // storage和余额使用的合约地址(可能代码和数据用的不是同一个合约的，因为delegate_call的存在)
	std::string api_name;
	std::string call_type;
};

/*
** 'per thread' state
*/
struct lua_State : vmgc::GcObject {
	const static vmgc::gc_type type = LUA_TTHREAD;
	int tt_ = LUA_TTHREAD;

    unsigned short nci;  /* number of items in 'ci' list */
    lu_byte status;
    StkId top;  /* first free slot in the stack */
    CallInfo *ci;  /* call info for current function */
    const Instruction *oldpc;  /* last pc traced */
    StkId stack_last;  /* last free slot in the stack */
    StkId stack;  /* stack base */
    UpVal *openupval;  /* list of open upvalues in this stack */
    struct lua_State *twups;  /* list of threads with open upvalues */
    struct lua_longjmp *errorJmp;  /* current error recover point */
    CallInfo base_ci;  /* CallInfo for first level (C calling Lua) */
    lua_Hook hook;
    ptrdiff_t errfunc;  /* current error handling function (stack index) */
    int stacksize;
    int basehookcount;
    int hookcount;
    unsigned short nny;  /* number of non-yieldable calls in stack */
    unsigned short nCcalls;  /* number of nested C calls */
    lu_byte hookmask;
    lu_byte allowhook;
    char compile_error[LUA_COMPILE_ERROR_MAX_LENGTH];
	char runerror[LUA_VM_EXCEPTION_STRNG_MAX_LENGTH];
    FILE *in;
    FILE *out;
    FILE *err;
    bool force_stopping;
	int exit_code;
    UvmStatePreProcessorFunction *preprocessor;
	vmgc::GcState *gc_state;
	lua_CFunction panic;  /* to be called in unprotected errors */
	lua_Alloc frealloc;  /* function to reallocate memory */
	void *ud;         /* auxiliary data to 'frealloc' */
	TValue l_registry;
	unsigned int seed;  /* randomized seed for hashes */
	//stringtable strt;  /* hash table for strings */

	const lua_Number *version;  /* pointer to version number */
	uvm_types::GcString *memerrmsg;  /* memory-error message */
	uvm_types::GcString *tmname[TM_N];  /* array with tag-method names */
	uvm_types::GcTable *mt[LUA_NUMTAGS];  /* metatables for basic types */
	uvm_types::GcString *strcache[STRCACHE_N][STRCACHE_M];  /* cache for strings in API */

	std::list<intptr_t> *contract_table_addresses;
	intptr_t allow_contract_modify; // contract table pointer whose properties can be modified now

	StkId evalstack; //for calulate
	StkId evalstacktop;//first free slot
	int evalstacksize;

	lua_VMState state;
	bool allow_debug;
	std::map<std::string, std::list<uint32_t> >* breakpoints; // contract_address => list of line_number
	std::stack<contract_info_stack_entry>* using_contract_id_stack;
	bool next_delegate_call_flag = false;
	OpCode call_op_msg;
	uint32_t ci_depth;
    
	int cbor_diff_state; // 0: not_set, 1: true, 2: false

	inline lua_State() :tt_(LUA_TTHREAD) {}
	virtual ~lua_State() {}
};

void *lua_malloc(lua_State *L, size_t size);

void *lua_calloc(lua_State *L, size_t element_count, size_t element_size);

void* lua_realloc(lua_State *L, void* addr, size_t old_size, size_t new_size);

void lua_free(lua_State *L, void *address);


#define state_G(L)	(L->l_G)

#define cast_u(o)	lua_cast(union GCUnion *, (o))

/* macros to convert a GCObject into a specific value */
#define gco2ts(o)  \
	check_exp(novariant((o)->tt) == LUA_TSTRING, (((uvm_types::GcString*)(o))))
#define gco2u(o)  check_exp((o)->tt == LUA_TUSERDATA, (((uvm_types::GcUserdata*)(o))))
#define gco2lcl(o)  check_exp((o)->tt == LUA_TLCL, ((uvm_types::GcLClosure*)(o)))
#define gco2ccl(o)  check_exp((o)->tt == LUA_TCCL, ((uvm_types::GcCClosure*)(o)))
#define gco2cl(o)  \
	check_exp(novariant((o)->tt) == LUA_TFUNCTION, ((uvm_types::GcClosure*)(o)))
#define gco2t(o)  check_exp((o)->tt == LUA_TTABLE, (((uvm_types::GcTable*)(o))))
#define gco2p(o)  check_exp((o)->tt == LUA_TPROTO, (((uvm_types::GcProto*)(o))))
#define gco2th(o)  check_exp((o)->tt == LUA_TTHREAD, ((lua_State*)(o)))


/* macro to convert a Lua object into a GCObject */
#define obj2gco(v) \
	check_exp(novariant((v)->tt) < LUA_TDEADKEY, (&(cast_u(v)->gc)))

/* macro to convert a Lua object into a vmgc::GcObject */
#define obj2vmgco(v) \
	check_exp(novariant((v)->tt_) < LUA_TDEADKEY, (&(v->gco)))


/* actual number of total bytes allocated */
#define gettotalbytes(g)	lua_cast(lu_mem, (g)->totalbytes + (g)->GCdebt)

LUAI_FUNC void luaE_setdebt(l_mem debt);
LUAI_FUNC CallInfo *luaE_extendCI(lua_State *L);
LUAI_FUNC void luaE_freeCI(lua_State *L);
LUAI_FUNC void luaE_shrinkCI(lua_State *L);


#endif

