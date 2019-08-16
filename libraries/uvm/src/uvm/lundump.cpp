/*
** $Id: lundump.c,v 2.44 2015/11/02 16:09:30 roberto Exp $
** load precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#define lundump_cpp
#define LUA_CORE

#include <uvm/lprefix.h>


#include <string.h>

#include <uvm/lua.h>

#include <uvm/ldebug.h>
#include <uvm/ldo.h>
#include <uvm/lfunc.h>
#include <uvm/lmem.h>
#include <uvm/lobject.h>
#include <uvm/lstring.h>
#include <uvm/lundump.h>
#include <uvm/lzio.h>
#include <uvm/uvm_lib.h>

using uvm::lua::api::global_uvm_chain_api;


#if !defined(luai_verifycode)
#define luai_verifycode(L,b,f)  /* empty */
#endif


typedef struct {
    lua_State *L;
    ZIO *Z;
    const char *name;
} LoadState;


static l_noret error(LoadState *S, const char *why) {
    luaO_pushfstring(S->L, "%s: %s precompiled chunk", S->name, why);
	global_uvm_chain_api->throw_exception(S->L, UVM_API_SIMPLE_ERROR, "%s: %s precompiled chunk", S->name, why);
    luaD_throw(S->L, LUA_ERRSYNTAX);
}


/*
** All high-level loads go through LoadVector; you can change it to
** adapt to the endianness of the input
*/
#define LoadVector(S,b,n)	LoadBlock(S,b,(n)*sizeof((b)[0]))

static void LoadBlock(LoadState *S, void *b, size_t size) {
    if (luaZ_read(S->Z, b, size) != 0)
        error(S, "truncated");
}


#define LoadVar(S,x)		LoadVector(S,&x,1)


static lu_byte LoadByte(LoadState *S) {
    lu_byte x;
    LoadVar(S, x);
    return x;
}


static int LoadInt(LoadState *S) {
    int32_t x;
    LoadVar(S, x);
    return x;
}


static lua_Number LoadNumber(LoadState *S) {
    lua_Number x;
    LoadVar(S, x);
    return x;
}


static lua_Integer LoadInteger(LoadState *S) {
    lua_Integer x;
    LoadVar(S, x);
    return x;
}


static uvm_types::GcString *LoadString(LoadState *S) {
    uint64_t size = LoadByte(S);
    if (size == 0xFF)
        LoadVar(S, size);
    if (size == 0)
        return nullptr;
    else if (--size <= LUAI_MAXSHORTLEN) {  /* short string? */
        char buff[LUAI_MAXSHORTLEN];
        LoadVector(S, buff, size);
        return luaS_newlstr(S->L, buff, size);
    }
    else {  /* long string */
        auto ts = luaS_createlngstrobj(S->L, size);
        LoadVector(S, getstr(ts), size);  /* load directly in final place */
        return ts;
    }
}

const int g_sizelimit = 1024 * 1024 * 500;

static bool LoadCode(LoadState *S, uvm_types::GcProto *f) {
    int n = LoadInt(S);
	if (n > g_sizelimit)
	{
		return false;
	}
	f->codes.resize(n);
	for (auto i = 0; i < n; i++) {
		f->codes[i] = 0;
	}
    LoadVector(S, f->codes.data(), n);
	return true;
}


static void LoadFunction(LoadState *S, uvm_types::GcProto *f, uvm_types::GcString *psource);

// 500M
const int g_constantslimit = 1024 * 1024 * 500;

static bool LoadConstants(LoadState *S, uvm_types::GcProto *f) {
    int i;
    int n = LoadInt(S);
	if (n > g_constantslimit)
	{
		return false;
	}
	f->ks.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->ks[i], 0x0, sizeof(f->ks[i]));
	}
    for (i = 0; i < n; i++)
        setnilvalue(&f->ks[i]);
    for (i = 0; i < n; i++) {
        TValue *o = &f->ks[i];
        int t = LoadByte(S);
        switch (t) {
        case LUA_TNIL:
            setnilvalue(o);
            break;
        case LUA_TBOOLEAN:
            setbvalue(o, LoadByte(S));
            break;
        case LUA_TNUMFLT:
            setfltvalue(o, LoadNumber(S));
            break;
        case LUA_TNUMINT:
            setivalue(o, LoadInteger(S));
            break;
        case LUA_TSHRSTR:
        case LUA_TLNGSTR:
            setsvalue2n(S->L, o, LoadString(S));
            break;
        default:
            lua_assert(0);
        }
    }
	return true;
}


static bool LoadProtos(LoadState *S, uvm_types::GcProto *f) {
    int i;
    int n = LoadInt(S);
	if (n > g_sizelimit)
	{
		return false;
	}
	f->ps.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->ps[i], 0x0, sizeof(f->ps[i]));
	}
    for (i = 0; i < n; i++)
        f->ps[i] = nullptr;
    for (i = 0; i < n; i++) {
        f->ps[i] = luaF_newproto(S->L);
        LoadFunction(S, f->ps[i], f->source);
    }
	return true;
}


static bool LoadUpvalues(LoadState *S, uvm_types::GcProto *f) {
    int i, n;
    n = LoadInt(S);
	if (n > g_sizelimit)
	{
		return false;
	}
	f->upvalues.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->upvalues[i], 0x0, sizeof(f->upvalues[i]));
	}
    for (i = 0; i < n; i++)
        f->upvalues[i].name = nullptr;
    for (i = 0; i < n; i++) {
        f->upvalues[i].instack = LoadByte(S);
        f->upvalues[i].idx = LoadByte(S);
    }
	return true;
}


// 500M
const int g_lineslimit = 1024 * 1024 * 500;

static bool LoadDebug(LoadState *S, uvm_types::GcProto *f) {
    int i, n;
    n = LoadInt(S);
	if (n > g_lineslimit)
	{
		return false;
	}
	f->lineinfos.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->lineinfos[i], 0x0, sizeof(f->lineinfos[i]));
	}
    LoadVector(S, f->lineinfos.data(), n);
    n = LoadInt(S);
	f->locvars.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->locvars[i], 0x0, sizeof(f->locvars[i]));
	};
    for (i = 0; i < n; i++)
        f->locvars[i].varname = nullptr;
    for (i = 0; i < n; i++) {
        f->locvars[i].varname = LoadString(S);
        f->locvars[i].startpc = LoadInt(S);
        f->locvars[i].endpc = LoadInt(S);
    }
    n = LoadInt(S);
    for (i = 0; i < n; i++)
        f->upvalues[i].name = LoadString(S);
	return true;
}


static void LoadFunction(LoadState *S, uvm_types::GcProto *f, uvm_types::GcString *psource) {
    f->source = LoadString(S);
    if (f->source == nullptr)  /* no source in dump? */
        f->source = psource;  /* reuse parent's source */
    f->linedefined = LoadInt(S);
    f->lastlinedefined = LoadInt(S);
    f->numparams = LoadByte(S);
    f->is_vararg = LoadByte(S);
    f->maxstacksize = LoadByte(S);
	if (!LoadCode(S, f))
	{
		return;
	}
	if (!LoadConstants(S, f))
	{
		return;
	}
	if (!LoadUpvalues(S, f))
	{
		return;
	}
	if (!LoadProtos(S, f))
	{
		return;
	}
    LoadDebug(S, f);
}


static void checkliteral(LoadState *S, const char *s, const char *msg) {
    char buff[sizeof(LUA_SIGNATURE) + sizeof(LUAC_DATA)]; /* larger than both */
    size_t len = strlen(s);
    LoadVector(S, buff, len);
    if (memcmp(s, buff, len) != 0)
        error(S, msg);
}


static void fchecksize(LoadState *S, size_t size, const char *tname) {
    if (LoadByte(S) != size)
        error(S, luaO_pushfstring(S->L, "%s size mismatch in", tname));
}


#define checksize(S,t)	fchecksize(S,sizeof(t),#t)

static void checkHeader(LoadState *S) {
    checkliteral(S, LUA_SIGNATURE + 1, "not a");  /* 1st char already checked */
    if (LoadByte(S) != LUAC_VERSION)
        error(S, "version mismatch in");
    if (LoadByte(S) != LUAC_FORMAT)
        error(S, "format mismatch in");
    checkliteral(S, LUAC_DATA, "corrupted");
    checksize(S, int32_t);
    checksize(S, LUA_SIZE_T_TYPE);
    checksize(S, Instruction);
    checksize(S, lua_Integer);
    checksize(S, lua_Number);
    if (LoadInteger(S) != LUAC_INT)
        error(S, "endianness mismatch in");
    if (LoadNumber(S) != LUAC_NUM)
        error(S, "float format mismatch in");
}


/*
** load precompiled chunk
*/
uvm_types::GcLClosure *luaU_undump(lua_State *L, ZIO *Z, const char *name) {
    LoadState S;
    uvm_types::GcLClosure *cl;
    if (*name == '@' || *name == '=')
        S.name = name + 1;
    else if (*name == LUA_SIGNATURE[0])
        S.name = "binary string";
    else
        S.name = name;
    S.L = L;
    S.Z = Z;
    checkHeader(&S);
	if (strlen(L->runerror) > 0 || strlen(L->compile_error) > 0)
		return nullptr;
    cl = luaF_newLclosure(L, LoadByte(&S));
    setclLvalue(L, L->top, cl);
    luaD_inctop(L);
    cl->p = luaF_newproto(L);
    LoadFunction(&S, cl->p, nullptr);
    lua_assert(cl->nupvalues == cl->p->upvalues.size());
    luai_verifycode(L, buff, cl->p);
    return cl;
}
