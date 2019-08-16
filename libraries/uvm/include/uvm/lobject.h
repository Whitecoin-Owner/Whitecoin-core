/*
** $Id: lobject.h,v 2.116 2015/11/03 18:33:10 roberto Exp $
** Type definitions for Lua objects
** See Copyright Notice in lua.h
*/


#ifndef lobject_h
#define lobject_h


#include <stdarg.h>
#include <map>
#include <algorithm>


#include "uvm/llimits.h"
#include "uvm/lua.h"
#include <vmgc/vmgc.h>


/*
** Extra tags for non-values
*/
#define LUA_TPROTO	LUA_NUMTAGS		/* function prototypes */
#define LUA_TDEADKEY	(LUA_NUMTAGS+1)		/* removed keys in tables */

/*
** number of all possible tags (including LUA_TNONE but excluding DEADKEY)
*/
#define LUA_TOTALTAGS	(LUA_TPROTO + 2)


/*
** tags for Tagged Values have the following use of bits:
** bits 0-3: actual tag (a LUA_T* value)
** bits 4-5: variant bits
** bit 6: whether value is collectable
*/


/*
** LUA_TFUNCTION variants:
** 0 - Lua function
** 1 - light C function
** 2 - regular C function (closure)
*/

/* Variant tags for functions */
#define LUA_TLCL	(LUA_TFUNCTION | (0 << 4))  /* Lua closure */
#define LUA_TLCF	(LUA_TFUNCTION | (1 << 4))  /* light C function */
#define LUA_TCCL	(LUA_TFUNCTION | (2 << 4))  /* C closure */


/* Variant tags for strings */
#define LUA_TSHRSTR	(LUA_TSTRING | (0 << 4))  /* short strings */
#define LUA_TLNGSTR	(LUA_TSTRING | (1 << 4))  /* long strings */


/* Variant tags for numbers */
#define LUA_TNUMFLT	(LUA_TNUMBER | (0 << 4))  /* float numbers */
#define LUA_TNUMINT	(LUA_TNUMBER | (1 << 4))  /* integer numbers */


/* Bit mark for collectable types */
#define BIT_ISCOLLECTABLE	(1 << 6)

/* mark a tag as collectable */
#define ctb(t)			((t) | BIT_ISCOLLECTABLE)

namespace uvm_types {
	struct GcString;
	struct GcTable;
}




/*
** Tagged Values. This is the basic representation of values in Lua,
** an actual value plus a tag with its type.
*/

/*
** Union of all Lua values
*/
typedef union Value {
	vmgc::GcObject* gco;
    void *p;         /* light userdata */
    int b;           /* booleans */
    lua_CFunction f; /* light C functions */
    lua_Integer i;   /* integer numbers */
    lua_Number n;    /* float numbers */
} Value;


#define TValuefields	Value value_; int tt_


typedef struct lua_TValue {
    TValuefields;
} TValue;



/* macro defining a nil value */
#define NILCONSTANT	{nullptr}, LUA_TNIL


#define val_(o)		((o)->value_)


/* raw type tag of a TValue */
#define rttype(o)	((o)->tt_)

/* tag with no variants (bits 0-3) */
#define novariant(x)	((x) & 0x0F)

/* type tag of a TValue (bits 0-3 for tags + variant bits 4-5) */
#define ttype(o)	(rttype(o) & 0x3F)

/* type tag of a TValue with no variants (bits 0-3) */
#define ttnov(o)	(novariant(rttype(o)))


/* Macros to test type */
#define checktag(o,t)		(rttype(o) == (t))
#define checktype(o,t)		(ttnov(o) == (t))
#define ttisnumber(o)		checktype((o), LUA_TNUMBER)
#define ttisfloat(o)		checktag((o), LUA_TNUMFLT)
#define ttisinteger(o)		checktag((o), LUA_TNUMINT)
#define ttisnil(o)		checktag((o), LUA_TNIL)
#define ttisboolean(o)		checktag((o), LUA_TBOOLEAN)
#define ttislightuserdata(o)	checktag((o), LUA_TLIGHTUSERDATA)
#define ttisstring(o)		checktype((o), LUA_TSTRING)
#define ttisshrstring(o)	checktag((o), ctb(LUA_TSHRSTR))
#define ttislngstring(o)	checktag((o), ctb(LUA_TLNGSTR))
#define ttistable(o)		checktag((o), ctb(LUA_TTABLE))
#define ttisfunction(o)		checktype(o, LUA_TFUNCTION)
#define ttisclosure(o)		((rttype(o) & 0x1F) == LUA_TFUNCTION)
#define ttisCclosure(o)		checktag((o), ctb(LUA_TCCL))
#define ttisLclosure(o)		checktag((o), ctb(LUA_TLCL))
#define ttislcf(o)		checktag((o), LUA_TLCF)
#define ttisfulluserdata(o)	checktag((o), ctb(LUA_TUSERDATA))
#define ttisthread(o)		checktag((o), ctb(LUA_TTHREAD))
#define ttisdeadkey(o)		checktag((o), LUA_TDEADKEY)


/* Macros to access values */
#define ivalue(o)	check_exp(ttisinteger(o), val_(o).i)
#define fltvalue(o)	check_exp(ttisfloat(o), val_(o).n)
#define nvalue(o)	check_exp(ttisnumber(o), \
	(ttisinteger(o) ? cast_num(ivalue(o)) : fltvalue(o)))
#define gcvalue(o)	check_exp(iscollectable(o), val_(o).gco)
#define pvalue(o)	check_exp(ttislightuserdata(o), val_(o).p)
#define tsvalue(o)	check_exp(ttisstring(o), gco2ts(val_(o).gco))
#define uvalue(o)	check_exp(ttisfulluserdata(o), gco2u(val_(o).gco))
#define clvalue(o)	check_exp(ttisclosure(o), gco2cl(val_(o).gco))
#define clLvalue(o)	check_exp(ttisLclosure(o), gco2lcl(val_(o).gco))
#define clCvalue(o)	check_exp(ttisCclosure(o), gco2ccl(val_(o).gco))
#define fvalue(o)	check_exp(ttislcf(o), val_(o).f)
#define hvalue(o)	check_exp(ttistable(o), gco2t(val_(o).gco))
#define bvalue(o)	check_exp(ttisboolean(o), val_(o).b)
#define thvalue(o)	check_exp(ttisthread(o), gco2th(val_(o).gco))
/* a dead value may get the 'gc' field, but cannot access its contents */
#define deadvalue(o)	check_exp(ttisdeadkey(o), lua_cast(void *, val_(o).gc))

#define l_isfalse(o)	(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))


#define iscollectable(o)	(rttype(o) & BIT_ISCOLLECTABLE)


/* Macros for internal tests */
#define righttt(obj)		(ttype(obj) == gcvalue(obj)->tt)

#define checkliveness(L,obj) (0)

/* Macros to set values */
#define settt_(o,t)	((o)->tt_=(t))

#define setfltvalue(obj,x) \
  { TValue *io=(obj); val_(io).n=(x); settt_(io, LUA_TNUMFLT); }

#define chgfltvalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisfloat(io)); val_(io).n=(x); }

#define setivalue(obj,x) \
  { TValue *io=(obj); val_(io).i=(x); settt_(io, LUA_TNUMINT); }

#define chgivalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisinteger(io)); val_(io).i=(x); }

#define setnilvalue(obj) settt_(obj, LUA_TNIL)

#define setfvalue(obj,x) \
  { TValue *io=(obj); val_(io).f=(x); settt_(io, LUA_TLCF); }

#define setpvalue(obj,x) \
  { TValue *io=(obj); val_(io).p=(x); settt_(io, LUA_TLIGHTUSERDATA); }

#define setbvalue(obj,x) \
  { TValue *io=(obj); val_(io).b=(x); settt_(io, LUA_TBOOLEAN); }

#define setgcovalue(L,obj,x) \
  { TValue *io = (obj); GCObject *i_g=(x); \
    val_(io).gc = i_g; settt_(io, ctb(i_g->tt)); }

//#define setsvalue(L,obj,x) \
//  { TValue *io = (obj); TString *x_ = (x); \
//    val_(io).gc = obj2gco(x_); settt_(io, ctb(x_->tt)); \
//    checkliveness(L,io); }

#define setsvalue(L,obj,x) \
  { TValue *io = (obj); uvm_types::GcString *x_ = (x); \
    val_(io).gco = x_; settt_(io, ctb(x_->tt_)); \
    checkliveness(L,io); }

#define setuvalue(L,obj,x) \
  { TValue *io = (obj); uvm_types::GcUserdata *x_ = (x); \
    val_(io).gco = x_; settt_(io, ctb(LUA_TUSERDATA)); \
    checkliveness(L,io); }

#define setthvalue(L,obj,x) \
  { TValue *io = (obj); lua_State *x_ = (x); \
    val_(io).gco = x_; settt_(io, ctb(LUA_TTHREAD)); \
    checkliveness(L,io); }

#define setclLvalue(L,obj,x) \
  { TValue *io = (obj); uvm_types::GcLClosure *x_ = (x); \
    val_(io).gco = x_; settt_(io, ctb(LUA_TLCL)); \
    checkliveness(L,io); }

#define setclCvalue(L,obj,x) \
  { TValue *io = (obj); uvm_types::GcCClosure *x_ = (x); \
    val_(io).gco = (x_); settt_(io, ctb(LUA_TCCL)); \
    checkliveness(L,io); }

#define sethvalue(L,obj,x) \
  { TValue *io = (obj); uvm_types::GcTable *x_ = (x); \
    val_(io).gco = x_; settt_(io, ctb(LUA_TTABLE)); \
    checkliveness(L,io); }

#define setdeadvalue(obj)	settt_(obj, LUA_TDEADKEY)



#define setobj(L,obj1,obj2) \
	{ TValue *io1=(obj1); *io1 = *(obj2); \
	  (void)L; checkliveness(L,io1); }


/*
** different types of assignments, according to destination
*/

/* from stack to (same) stack */
#define setobjs2s	setobj
/* to stack (not from same stack) */
#define setobj2s	setobj
#define setsvalue2s	setsvalue
#define sethvalue2s	sethvalue
#define setptvalue2s	setptvalue
/* from table to same table */
#define setobjt2t	setobj
/* to new object */
#define setobj2n	setobj
#define setsvalue2n	setsvalue

/* to table (define it as an expression to be used in macros) */
#define setobj2t(L,o1,o2)  ((void)L, *(o1)=*(o2), checkliveness(L,(o1)))




/*
** {======================================================
** types and prototypes
** =======================================================
*/


typedef TValue *StkId;  /* index to stack elements */

namespace uvm_types {
	struct GcString;
	struct GcProto;
}

/*
** Get the actual string (array of bytes) from a 'TString'.
** (Access to 'extra' ensures that value is really a 'TString'.)
*/
#define getstr(ts)  \
  ((char*)((ts)->value.data()))


/* get the actual string (array of bytes) from a Lua value */
#define svalue(o)       getstr(tsvalue(o))

/* get string length from 'TString *s' */
#define tsslen(s)	((s)->tt == LUA_TSHRSTR ? (s)->value.size() : (s)->value.size())

/* get string length from 'TValue *o' */
#define vslen(o)	tsslen(tsvalue(o))


/*
**  Get the address of memory block inside 'Udata'.
** (Access to 'ttuv_' ensures that value is really a 'Udata'.)
*/
#define getudatamem(u)  \
  check_exp(sizeof((u)->ttuv_), (lua_cast(char*, (u)->gc_value)))

#define setuservalue(L,u,o) \
	{ const TValue *io=(o); uvm_types::GcUserdata *iu = (u); \
	  iu->user_ = io->value_; iu->ttuv_ = rttype(io); \
	  checkliveness(L,io); }


#define getuservalue(L,u,o) \
	{ TValue *io=(o); auto *iu = (u); \
	  io->value_.gco = iu; settt_(io, iu->tt_); \
	  checkliveness(L,io); }


/*
** Description of an upvalue for function prototypes
*/
typedef struct Upvaldesc {
	uvm_types::GcString *name;  /* upvalue name (for debug information) */
    lu_byte instack;  /* whether it is in stack (register) */
    lu_byte idx;  /* index of upvalue (in stack or in outer function's list) */
} Upvaldesc;


/*
** Description of a local variable for function prototypes
** (used for debug information)
*/
typedef struct LocVar {
    uvm_types::GcString *varname;
    int startpc;  /* first point where variable is active */
    int endpc;    /* first point where variable is dead */
    int vartype;
} LocVar;



/*
** Lua Upvalues
*/
typedef struct UpVal UpVal;


#define isLfunction(o)	ttisLclosure(o)

#define getproto(o)	(clLvalue(o)->p)

/*
** 'module' operation for hashing (size is always a power of 2)
*/
#define lmod(s,size) \
	(check_exp((size&(size-1))==0, (lua_cast(int, (s) & ((size)-1)))))


#define twoto(x)	(1<<(x))
#define sizenode(t)	(twoto((t)->lsizenode))


/*
** (address of) a fixed nil value
*/
#define luaO_nilobject		(&luaO_nilobject_)


LUAI_DDEC const TValue luaO_nilobject_;

/* size of buffer for 'luaO_utf8esc' function */
#define UTF8BUFFSZ	8

LUAI_FUNC int luaO_int2fb(unsigned int x);
LUAI_FUNC int luaO_fb2int(int x);
LUAI_FUNC int luaO_utf8esc(char *buff, unsigned long x);
LUAI_FUNC int luaO_ceillog2(unsigned int x);
LUAI_FUNC void luaO_arith(lua_State *L, int op, const TValue *p1,
    const TValue *p2, TValue *res);
LUAI_FUNC size_t luaO_str2num(const char *s, TValue *o);
LUAI_FUNC int luaO_hexavalue(int c);
LUAI_FUNC void luaO_tostring(lua_State *L, StkId obj);
LUAI_FUNC const char *luaO_pushvfstring(lua_State *L, const char *fmt,
    va_list argp);
LUAI_FUNC const char *luaO_pushfstring(lua_State *L, const char *fmt, ...);
LUAI_FUNC void luaO_chunkid(char *out, const char *source, size_t len);

LUAI_FUNC size_t lua_number2str_impl(char* s, size_t sz, lua_Number n);


unsigned int luaS_hash(const char *str, size_t l, unsigned int seed);

// vmgc object types
namespace uvm_types {
	struct GcString : vmgc::GcObject
	{
		const static vmgc::gc_type type = LUA_TLNGSTR;
		int tt_ = LUA_TLNGSTR;
		std::string value;
		lu_byte extra = 0;

		inline GcString() : tt_(LUA_TLNGSTR){ }

		virtual ~GcString() {}

		inline unsigned int hash() const {
			return luaS_hash(value.data(), value.size(), 1);
		}
	};
	struct GcUserdata : vmgc::GcObject
	{
		const static vmgc::gc_type type = LUA_TUSERDATA;
		int tt_ = LUA_TUSERDATA;
		void* gc_value = nullptr; // user value managed in vmgc
		uvm_types::GcTable *metatable;
		size_t len;  /* number of bytes */
		int ttuv_;
		Value user_;

		inline GcUserdata() : tt_(LUA_TUSERDATA), gc_value(nullptr), metatable(0), len(0), ttuv_(LUA_TNIL) {
			this->user_.gco = nullptr;
		}

		virtual ~GcUserdata() {}
	};
	struct GcClosure : vmgc::GcObject 
	{
		virtual ~GcClosure() {}

		virtual vmgc::gc_type tt_value() const = 0;
		virtual lu_byte nupvalues_count() const = 0;
	};
	struct GcProto;

	struct GcLClosure : GcClosure
	{
		const static vmgc::gc_type type = LUA_TLCL;
		lu_byte nupvalues;
		int tt_ = LUA_TLCL;
		GcProto *p;
		std::vector<UpVal*> upvals;

		inline GcLClosure() : nupvalues(0), tt_(LUA_TLCL), p(nullptr) {}

		virtual ~GcLClosure() {}
		inline virtual vmgc::gc_type tt_value() const { return tt_; }
		inline virtual lu_byte nupvalues_count() const { return nupvalues; }
	};
	struct GcCClosure : GcClosure
	{
		const static vmgc::gc_type type = LUA_TCCL;
		lu_byte nupvalues;
		int tt_ = LUA_TCCL;
		lua_CFunction f;
		std::vector<TValue> upvalue;

		inline GcCClosure() : nupvalues(0), tt_(LUA_TCCL), f(nullptr) {}

		virtual ~GcCClosure() {}
		inline virtual vmgc::gc_type tt_value() const { return tt_; }
		inline virtual lu_byte nupvalues_count() const { return nupvalues; }
	};
	struct table_sort_comparator
	{
	public:
		bool operator()(const TValue& x, const TValue& y) const;
	};
	struct GcTable : vmgc::GcObject
	{
		typedef TValue GcTableItemType;
		const static vmgc::gc_type type = LUA_TTABLE;
		int tt_ = LUA_TTABLE;
		std::map<TValue, GcTableItemType, table_sort_comparator> entries; // must use map, not unordered_map
		std::map<std::string, TValue> keys;
		std::vector<GcTableItemType> array;
		GcTable* metatable;
		lu_byte flags; // flag to mask meta methods
		bool isOnlyRead = false; 
		inline GcTable() : metatable(nullptr), flags(0) { }
		virtual ~GcTable() {}
	};
	struct GcProto : vmgc::GcObject
	{
		typedef TValue GcTableItemType;
		const static vmgc::gc_type type = LUA_TPROTO;
		int tt_ = LUA_TPROTO;

		lu_byte numparams;  /* number of fixed parameters */
		lu_byte is_vararg;  /* 2: declared vararg; 1: uses vararg */
		lu_byte maxstacksize;  /* number of registers needed by this function */
		int linedefined;  /* debug information  */
		int lastlinedefined;  /* debug information  */
		std::vector<TValue> ks;  /* constants used by the function */
		std::vector<Instruction> codes;  /* opcodes */
		std::vector<GcProto*> ps;  /* functions defined inside the function */
		std::vector<int> lineinfos;  /* map from opcodes to source lines (debug information) */
		std::vector<LocVar> locvars;  /* information about local variables (debug information) */
		std::vector<Upvaldesc> upvalues;  /* upvalue information */
		struct GcLClosure *cache;  /* last-created closure with this prototype */
		GcString  *source;  /* used for debug information */

		inline GcProto() : numparams(0), is_vararg(0), maxstacksize(0), linedefined(0)
			, cache(nullptr), source(nullptr)
		{ }
		virtual ~GcProto() {}
	};
	// TODO: Contract

}



#endif

