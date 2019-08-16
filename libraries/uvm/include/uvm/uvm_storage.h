#ifndef uvm_storage_h
#define uvm_storage_h

#include <uvm/lprefix.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <list>
#include <set>
#include <unordered_map>
#include <memory>

#include <uvm/lua.h>
#include <uvm/lauxlib.h>
#include <uvm/lualib.h>
#include <uvm/lapi.h>

#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>

namespace uvm
{
	namespace lib
	{
		// int uvmlib_get_storage(lua_State *L);
		// implementation of uvm.get_storage in contract
		int uvmlib_get_storage_impl(lua_State *L,
			const char *contract_id, const char *name, const char* fast_map_key, bool is_fast_map);

		// int uvmlib_set_storage(lua_State *L);

		int uvmlib_set_storage_impl(lua_State *L,
			const char *contract_id, const char *name, const char* fast_map_key, bool is_fast_map, int value_index);
	}
}

#endif
