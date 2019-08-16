#define ljsonlib_cpp

#include <uvm/lprefix.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <cassert>

#include <uvm/lua.h>

#include <uvm/lauxlib.h>
#include <uvm/lualib.h>
#include <uvm/uvm_tokenparser.h>
#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>
#include <uvm/uvm_lutil.h>

#include <uvm/json_reader.h>

using uvm::lua::api::global_uvm_chain_api;

using namespace uvm::parser;

static UvmStorageValue nil_storage_value()
{
	UvmStorageValue value;
	value.type = uvm::blockchain::StorageValueTypes::storage_value_null;
	value.value.int_value = 0;
	return value;
}


static int json_to_lua(lua_State *L)
{
	if (lua_gettop(L) < 1)
		return 0;
	if (!lua_isstring(L, 1))
		return 0;
	std::string json_str(luaL_checkstring(L, 1));
	auto json_str_size = json_str.size();
	size_t little_large_size = 10000;
	size_t very_large_size = 100000;
	if (json_str_size > little_large_size) {
		auto json_gas_punishment_fork_height = global_uvm_chain_api->get_fork_height(L, "JSON_GAS_PUNISHMENT");
		if (json_gas_punishment_fork_height >= 0 && global_uvm_chain_api->get_header_block_num(L) >= json_gas_punishment_fork_height) {
			auto common_extra_gas = 10 * json_str_size;
			if (json_str_size > little_large_size) {
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, common_extra_gas - 1);
			}
			else if (json_str_size > very_large_size) {
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, 10 * common_extra_gas - 1);
			}
		}
	}
	auto json_parser = std::make_shared<Json_Reader>();
	UvmStorageValue root;
	if (json_parser->parse(L, json_str, &root)) {
		lua_push_storage_value(L, root);
		return 1;
	}
	else {
		global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "parse json error(%s)", json_parser->error.message_.c_str());
		return 0;
	}

}

static int lua_to_json(lua_State *L)
{
	if (lua_gettop(L) < 1)
		return 0;
	auto value = luaL_tojsonstring(L, 1, nullptr);
	auto json_str_size = strlen(value);
	size_t little_large_size = 10000;
	size_t very_large_size = 100000;
	if (json_str_size > little_large_size) {
		auto json_gas_punishment_fork_height = global_uvm_chain_api->get_fork_height(L, "JSON_GAS_PUNISHMENT");
		if (json_gas_punishment_fork_height >= 0 && global_uvm_chain_api->get_header_block_num(L) >= json_gas_punishment_fork_height) {
			auto common_extra_gas = 10 * json_str_size;
			if (json_str_size > little_large_size) {
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, common_extra_gas - 1);
			}
			else if (json_str_size > very_large_size) {
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, 10 * common_extra_gas - 1);
			}
		}
	}
	lua_pushstring(L, value);
	return 1;
}

static const luaL_Reg dblib[] = {
	{ "loads", json_to_lua },
{ "dumps", lua_to_json },
{ nullptr, nullptr }
};


LUAMOD_API int luaopen_json2(lua_State *L) {
	luaL_newlib(L, dblib);
	return 1;
}

