/*
** $Id: ltable.c,v 2.117 2015/11/19 19:16:22 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#define ltable_cpp
#define LUA_CORE

#include <uvm/lprefix.h>


/*
** Implementation of tables (aka arrays, objects, or hash tables).
** Tables keep its elements in two parts: an array part and a hash part.
** Non-negative integer keys are all candidates to be kept in the array
** part. The actual size of the array is the largest 'n' such that
** more than half the slots between 1 and n are in use.
** Hash uses a mix of chained scatter table with Brent's variation.
** A main invariant of these tables is that, if an element is not
** in its main position (i.e. the 'original' position that its hash gives
** to it), then the colliding element is in its own main position.
** Hence even when the load factor reaches 100%, performance remains good.
*/

#include <math.h>
#include <limits.h>

#include <map>
#include <vector>

#include <uvm/lua.h>

#include <uvm/ldebug.h>
#include <uvm/ldo.h>
#include <uvm/lgc.h>
#include <uvm/lmem.h>
#include <uvm/lobject.h>
#include <uvm/lstate.h>
#include <uvm/lstring.h>
#include <uvm/ltable.h>
#include <uvm/lvm.h>
#include <uvm/uvm_lutil.h>
#include <uvm/uvm_api.h>


/*
** Maximum size of array part (MAXASIZE) is 2^MAXABITS. MAXABITS is
** the largest integer such that MAXASIZE fits in an unsigned int.
*/
#define MAXABITS	cast_int(sizeof(int) * CHAR_BIT - 1)
#define MAXASIZE	(1u << MAXABITS)

/*
** Maximum size of hash part is 2^MAXHBITS. MAXHBITS is the largest
** integer such that 2^MAXHBITS fits in a signed int. (Note that the
** maximum number of elements in a table, 2^MAXABITS + 2^MAXHBITS, still
** fits comfortably in an unsigned int.)
*/
#define MAXHBITS	(MAXABITS - 1)

/*
** returns the index for 'key' if 'key' is an appropriate key to live in
** the array part of the table, 0 otherwise.
*/
static unsigned int arrayindex(const TValue *key) {
    if (ttisinteger(key)) {
        lua_Integer k = ivalue(key);
        if (0 < k && (lua_Unsigned)k <= MAXASIZE)
            return lua_cast(unsigned int, k);  /* 'key' is an appropriate array index */
    }
    return 0;  /* 'key' did not match some condition */
}

static std::string lua_value_to_str(const TValue *key)
{
	if (ttisstring(key))
	{
		return std::string(svalue(key));
	}
	else if (ttisinteger(key))
	{
		return std::to_string((lua_Integer)nvalue(key));
	}
	else if (ttisnumber(key))
	{
		return std::to_string(nvalue(key));
	}
	else
		return "";
}


static bool val_to_table_key(const TValue* key, std::string& out) {
	if (ttisnil(key)) {
		return false;
	}
	else if (ttisfloat(key)) {
		lua_Integer k;
		if (luaV_tointeger(key, &k, 0)) {
			out = std::to_string(k);
			return true;
		}
		else {
			return false;
		}
	}
	else if (ttisinteger(key)) {
		lua_Integer k;
		if (luaV_tointeger(key, &k, 0)) {
			out = std::to_string(k);
			return true;
		}
		else {
			return false;
		}
	}
	else if (ttisstring(key)) {
		out = getstr(gco2ts(key->value_.gco));
		return true;
	}
	else if (ttislightuserdata(key)) {
		out = std::string("$lightuserdata@") + std::to_string(intptr_t(key->value_.p));
		return true;
	}
	else {
		return false;
	}
}


static bool findindex_of_sorted_table(lua_State *L, uvm_types::GcTable *t, StkId key, unsigned int* array_index, std::string* map_key, bool* is_map_key) {
	unsigned int i = arrayindex(key);
	/*if (i == 0 && ttisnil(key)) {
		*array_index = 0;
		*is_map_key = false;
		return true;
	}*/
	if (i != 0 && i <= t->array.size())  /* is 'key' inside array part? */
	{
		*array_index = i;
		*is_map_key = false;
		return true;  /* yes; that's the index */
	}
	else {
		std::string key_str;
		if (!val_to_table_key(key, key_str)) {
			return false;
		}
		auto key_it = t->keys.find(key_str);
		if (key_it == t->keys.end())
			return false;
		*map_key = key_str;
		*is_map_key = true;
		return true;
	}
}

static const char* NOT_FOUND_KEY = "$NOT_FOUND$";


int luaH_next(lua_State *L, uvm_types::GcTable *t, StkId key) {
	unsigned int array_index;
	std::string map_key;
	bool is_map_key;
	bool use_first_map_key = false;
	// array_index is index of key£¬1-based£¬0 means not found
    bool found_key = findindex_of_sorted_table(L, t, key, &array_index, &map_key, &is_map_key);  /* find original element */
	if (nullptr == t) {
		return 0;
	}
	if (ttisnil(key)) {
		array_index = 0;
		is_map_key = false;
	}
	auto array_part_size = t->array.size();
	if (!is_map_key) {
		// key in array part
		for (auto i = array_index; i < array_part_size; i++) {
			if (!ttisnil(&t->array[i])) {  /* a non-nil value? */
				setivalue(key, i + 1);
				setobj2s(L, key + 1, &t->array[i]);
				return 1;
			}
		}
		use_first_map_key = true;
	}
	// key in map part
	auto item_value_it = t->entries.begin();
	if (!use_first_map_key) {
		auto it = t->keys.find(map_key);
		if (it == t->keys.end()) {
			return 0;
		}
		const auto& item_key = it->second;
		item_value_it = t->entries.find(item_key);
		if (item_value_it != t->entries.end())
			item_value_it++;
	}
	do {
		if (item_value_it == t->entries.end())
			return 0;
		if (ttisnil(&item_value_it->second))
		{
			item_value_it++;
			continue;
		}
		if (ttisinteger(&item_value_it->first)) {
			setobj2s(L, key, &item_value_it->first);
			setobj2s(L, key + 1, &item_value_it->second);
			return 1;
		}

		std::string item_key_str;
		if (!val_to_table_key(&item_value_it->first, item_key_str)) {
			item_value_it++;
			continue;
		}
		auto s = luaS_new(L, item_key_str.c_str());
		if (!s) {
			return 0;
		}
		setsvalue(L, key, s);
		setobj2s(L, key + 1, &item_value_it->second);
		return 1;
	} while (true);
	
	return 0;
}

void luaH_resize(lua_State *L, uvm_types::GcTable *t, unsigned int nasize,
    unsigned int nhsize) {
    unsigned int i;
    int j;
    unsigned int oldasize = t->array.size();
	if (nasize > oldasize)  /* array part must grow? */
	{
		for (auto i = 0; i < nasize - oldasize; i++) {
			t->array.push_back(*luaO_nilobject);
		}
	}
	else {
		t->array.resize(nasize);
	}
}


void luaH_resizearray(lua_State *L, uvm_types::GcTable *t, unsigned int nasize) {
	int nsize = t->entries.size();
    luaH_resize(L, t, nasize, nsize);
}


/*
** }=============================================================
*/


uvm_types::GcTable *luaH_new(lua_State *L) {
	auto o = L->gc_state->gc_new_object<uvm_types::GcTable>();
    return o;
}


void luaH_free(lua_State *L, uvm_types::GcTable *t) {
	L->gc_state->gc_free(t);
}

void luaH_setisonlyread(lua_State *L, uvm_types::GcTable *t, bool isOnlyRead) {
	t->isOnlyRead = isOnlyRead;
}

/*
** inserts a new key into a hash table; first, check whether key's main
** position is free. If not, check whether colliding node is in its main
** position or not: if it is not, move colliding node to an empty place and
** put new key in its main position; otherwise (colliding node is in its main
** position), new key goes to an empty position.
*/
TValue *luaH_newkey(lua_State *L, uvm_types::GcTable *t, const TValue *key, bool allow_lightuserdata) {
    TValue aux;
	bool is_int = false;
	lua_Integer k;
    if (ttisnil(key)) luaG_runerror(L, "table index is nil");
    else if (ttisfloat(key) || ttisinteger(key)) {
        if (luaV_tointeger(key, &k, 0)) {  /* index is int? */
            setivalue(&aux, k);
            key = &aux;  /* insert it as an integer */
			is_int = true;
        }
        else if (luai_numisnan(fltvalue(key)))
            luaG_runerror(L, "table index is NaN");
	}if (!allow_lightuserdata) {
		// not support lightuserdata as key by default. need lightuserdata key sometimes because of lua_rawsetp
		if (ttislightuserdata(key)) {
			luaG_runerror(L, "invalid table key type");
			return nullptr;
		}
	}
	// if key is int and == len(array+1), newkey put to array part
	if (is_int && k == (t->array.size() + 1)) {
		t->array.push_back(*luaO_nilobject);
		return &t->array[k-1];
	}
	TValue key_obj(*key);
	std::string key_str;
	if (!val_to_table_key(key, key_str)) {
		luaG_runerror(L, "invalid table key type");
		return nullptr;
	}
	
	t->entries[key_obj] = *luaO_nilobject;
	t->keys[key_str] = key_obj;
	auto it = t->entries.find(key_obj);
	return &it->second;
}


/*
** search function for integers
*/
const TValue *luaH_getint(uvm_types::GcTable *t, lua_Integer key) {
    if (l_castS2U(key) - 1 < t->array.size())
        return &t->array[key - 1];
    else {
		const auto& key_str = std::to_string(key);
		auto key_obj_it = t->keys.find(key_str);
		if (key_obj_it == t->keys.end())
			return luaO_nilobject;
		const auto& key_obj = key_obj_it->second;
		auto val_it = t->entries.find(key_obj);
		if(val_it == t->entries.end())
			return luaO_nilobject;
		return &val_it->second;
    }
}


/*
** search function for short strings
*/
const TValue *luaH_getshortstr(uvm_types::GcTable *t, uvm_types::GcString *key) {
	std::string key_str = key->value;
	auto key_obj_it = t->keys.find(key_str);
	if (key_obj_it == t->keys.end())
		return luaO_nilobject;
	const auto& key_obj = key_obj_it->second;
	auto val_it = t->entries.find(key_obj);
	if (val_it == t->entries.end())
		return luaO_nilobject;
	return &val_it->second;
}


/*
** "Generic" get version. (Not that generic: not valid for integers,
** which may be in array part, nor for floats with integral values.)
*/
static const TValue *getgeneric(uvm_types::GcTable *t, const TValue *key) {
	std::string key_str;
	if (!val_to_table_key(key, key_str)) {
		return luaO_nilobject;
	}
	auto key_obj_it = t->keys.find(key_str);
	if (key_obj_it == t->keys.end())
		return luaO_nilobject;
	const auto& key_obj = key_obj_it->second;
	auto val_it = t->entries.find(key_obj);
	if (val_it == t->entries.end())
		return luaO_nilobject;
	return &val_it->second;
}


const TValue *luaH_getstr(uvm_types::GcTable *t, uvm_types::GcString *key) {
	if (key->tt == LUA_TSHRSTR)
		return luaH_getshortstr(t, key);
    else {  /* for long strings, use generic case */
        TValue ko;
        setsvalue(lua_cast(lua_State *, nullptr), &ko, key);
        return getgeneric(t, &ko);
    }
}


/*
** main search function
*/
const TValue *luaH_get(uvm_types::GcTable *t, const TValue *key) {
    switch (ttype(key)) {
    case LUA_TSHRSTR: return luaH_getshortstr(t, tsvalue(key));
	case LUA_TLNGSTR: return luaH_getstr(t, tsvalue(key));
    case LUA_TNUMINT: return luaH_getint(t, ivalue(key));
    case LUA_TNIL: return luaO_nilobject;
    case LUA_TNUMFLT: {
        lua_Integer k;
        if (luaV_tointeger(key, &k, 0)) /* index is int? */
            return luaH_getint(t, k);  /* use specialized version */
        /* else... */
    }  /* FALLTHROUGH */
    default:
        return getgeneric(t, key);
    }
}


/*
** beware: when using this function you probably need to check a GC
** barrier and invalidate the TM cache.
*/
TValue *luaH_set(lua_State *L, uvm_types::GcTable *t, const TValue *key, bool allow_lightuserdata) {
    const TValue *p = luaH_get(t, key);
    if (p != luaO_nilobject)
        return lua_cast(TValue *, p);
    else return luaH_newkey(L, t, key, allow_lightuserdata);
}


void luaH_setint(lua_State *L, uvm_types::GcTable *t, lua_Integer key, TValue *value) {
    const TValue *p = luaH_getint(t, key);
    TValue *cell;
    if (p != luaO_nilobject)
        cell = lua_cast(TValue *, p);
    else {
        TValue k;
        setivalue(&k, key);
        cell = luaH_newkey(L, t, &k);
    }
    setobj2t(L, cell, value);
}


static int unbound_search(uvm_types::GcTable *t, unsigned int j) {
    unsigned int i = j;  /* i is zero or a present index */
    j++;
    /* find 'i' and 'j' such that i is present and j is not */
    while (!ttisnil(luaH_getint(t, j))) {
        i = j;
        if (j > lua_cast(unsigned int, MAX_INT) / 2) {  /* overflow? */
            /* table was built with bad purposes: resort to linear search */
            i = 1;
            while (!ttisnil(luaH_getint(t, i))) i++;
            return i - 1;
        }
        j *= 2;
    }
    /* now do a binary search between them */
    while (j - i > 1) {
        unsigned int m = (i + j) / 2;
        if (ttisnil(luaH_getint(t, m))) j = m;
        else i = m;
    }
    return i;
}


/*
** Try to find a boundary in table 't'. A 'boundary' is an integer index
** such that t[i] is non-nil and t[i+1] is nil (and 0 if t[1] is nil).
*/
int luaH_getn(uvm_types::GcTable *t) {
    unsigned int j = t->array.size();
    if (j > 0 && ttisnil(&t->array[j - 1])) {
        /* there is a boundary in the array part: (binary) search for it */
        unsigned int i = 0;
        while (j - i > 1) {
            unsigned int m = (i + j) / 2;
            if (ttisnil(&t->array.at(m - 1))) j = m;
            else i = m;
        }
        return i;
    }
    /* else must find a boundary in hash part */
    else if (t->entries.empty())  /* hash part is empty? */
        return j;  /* that is easy... */
    else return unbound_search(t, j);
}



#if defined(LUA_DEBUG)

Node *luaH_mainposition(const Table *t, const TValue *key) {
    return mainposition(t, key);
}

int luaH_isdummy(Node *n) { return isdummy(n); }

#endif
