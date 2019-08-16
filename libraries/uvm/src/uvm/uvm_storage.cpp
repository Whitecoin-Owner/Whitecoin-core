#include <uvm/lprefix.h>

#include <stdlib.h>
#include <math.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>

#include <uvm/uvm_storage.h>
#include <jsondiff/jsondiff.h>
#include <jsondiff/exceptions.h>
#include <uvm/uvm_lib.h>

using uvm::lua::api::global_uvm_chain_api;

static UvmStorageTableReadList *get_or_init_storage_table_read_list(lua_State *L)
{
	UvmStateValueNode state_value_node = uvm::lua::lib::get_lua_state_value_node(L, LUA_STORAGE_READ_TABLES_KEY);
	UvmStorageTableReadList *list = nullptr;;
	if (state_value_node.type != LUA_STATE_VALUE_POINTER || nullptr == state_value_node.value.pointer_value)
	{
		list = (UvmStorageTableReadList*)lua_malloc(L, sizeof(UvmStorageTableReadList));
		if (!list)
			return nullptr;
		new (list)UvmStorageTableReadList();
		UvmStateValue value_to_store;
		value_to_store.pointer_value = list;
		uvm::lua::lib::set_lua_state_value(L, LUA_STORAGE_READ_TABLES_KEY, value_to_store, LUA_STATE_VALUE_POINTER);
	}
	else
	{
		list = (UvmStorageTableReadList*)state_value_node.value.pointer_value;
	}
	return list;
}

static struct UvmStorageValue get_last_storage_changed_value(lua_State *L, const char *contract_id,
	UvmStorageChangeList *list, const std::string &key, const std::string& fast_map_key, bool is_fast_map)
{
	struct UvmStorageValue nil_value;
	nil_value.type = uvm::blockchain::StorageValueTypes::storage_value_null;
	std::string contract_id_str(contract_id);
	auto post_when_read_table = [&](UvmStorageValue value) {
		if (lua_storage_is_table(value.type))
		{
			// when read a table, snapshot it and record it, when commit, merge to the changes
			UvmStorageTableReadList *table_read_list = get_or_init_storage_table_read_list(L);
			if (table_read_list)
			{
				for (auto it = table_read_list->begin(); it != table_read_list->end(); ++it)
				{
					if (it->contract_id == contract_id_str && it->key == key && it->fast_map_key==fast_map_key && it->is_fast_map == is_fast_map)
					{
						return;
					}
				}
				UvmStorageChangeItem change_item;
				change_item.contract_id = contract_id_str;
				change_item.key = key;
				change_item.fast_map_key = is_fast_map ? fast_map_key : "";
				change_item.is_fast_map = is_fast_map;
				change_item.before = value;
				table_read_list->push_back(change_item);
			}
		}
	};
	if (!list || list->size() < 1)
	{
		auto value = global_uvm_chain_api->get_storage_value_from_uvm_by_address(L, contract_id, key, fast_map_key, is_fast_map);
		post_when_read_table(value);
		// cache the value if it's the first time to read
		if (!list) {
			list = (UvmStorageChangeList*)lua_malloc(L, sizeof(UvmStorageChangeList));
			if (!list)
				return value;
			new (list)UvmStorageChangeList();
			UvmStateValue value_to_store;
			value_to_store.pointer_value = list;
			uvm::lua::lib::set_lua_state_value(L, LUA_STORAGE_CHANGELIST_KEY, value_to_store, LUA_STATE_VALUE_POINTER);
		}

		UvmStorageChangeItem change_item;
		change_item.before = value;
		change_item.after = value;
		change_item.contract_id = contract_id_str;
		change_item.key = key;
		change_item.fast_map_key = is_fast_map ? fast_map_key : "";
		change_item.is_fast_map = is_fast_map;
		list->push_back(change_item);

		return value;
	}
	for (auto it = list->rbegin(); it != list->rend(); ++it)
	{
		if (it->contract_id == contract_id_str && it->key == key && it->fast_map_key==fast_map_key && it->is_fast_map == is_fast_map)
			return it->after;
	}
	auto value = global_uvm_chain_api->get_storage_value_from_uvm_by_address(L, contract_id, key, fast_map_key, is_fast_map);
	post_when_read_table(value);
	return value;
}

static std::string get_contract_id_string_in_storage_operation(lua_State *L)
{
	return uvm::lua::lib::get_current_using_storage_contract_id(L);
}

static std::string global_key_for_storage_prop(const std::string& contract_id, const std::string& key, const std::string& fast_map_key, bool is_fast_map)
{
	return "gk_" + contract_id + "__" + key + "__" + fast_map_key + "__" + std::to_string(is_fast_map);
}

bool lua_push_storage_value(lua_State *L, const UvmStorageValue &value);
#define max_support_array_size 10000000  // max array size supported

static bool lua_push_storage_table_value(lua_State *L, UvmTableMap *map, int type)
{
	if (nullptr == L || nullptr == map)
		return false;
	lua_createtable(L, 0, 0);
	// when is array, push as array
	if (lua_storage_is_array((uvm::blockchain::StorageValueTypes) type))
	{
		// sort the unordered_map, then push items keys, 1,2,3,... one by one into the new table array
		auto count = 0;
		while (count < max_support_array_size)
		{
			++count;
			std::string key = std::to_string(count);
			if (map->find(key) == map->end())
				break;
			auto value = map->at(key);
			lua_push_storage_value(L, value);
			lua_seti(L, -2, count);
		}
	}
	else
	{
		for (auto it = map->begin(); it != map->end(); ++it)
		{
			const auto &key = it->first;
			UvmStorageValue value = it->second;
			lua_push_storage_value(L, value);
			lua_setfield(L, -2, key.c_str());
		}
	}
	return true;
}
bool lua_push_storage_value(lua_State *L, const UvmStorageValue &value)
{
	if (nullptr == L)
		return false;
	switch (value.type)
	{
	case uvm::blockchain::StorageValueTypes::storage_value_int: lua_pushinteger(L, value.value.int_value); break;
	case uvm::blockchain::StorageValueTypes::storage_value_bool: lua_pushboolean(L, value.value.bool_value); break;
	case uvm::blockchain::StorageValueTypes::storage_value_number: lua_pushnumber(L, value.value.number_value); break;
	case uvm::blockchain::StorageValueTypes::storage_value_null: lua_pushnil(L); break;
	case uvm::blockchain::StorageValueTypes::storage_value_string: lua_pushstring(L, value.value.string_value); break;
	case uvm::blockchain::StorageValueTypes::storage_value_stream: {
		auto stream = (uvm::lua::lib::UvmByteStream*) value.value.userdata_value;
		lua_pushlightuserdata(L, (void*)stream);
		luaL_getmetatable(L, "UvmByteStream_metatable");
		lua_setmetatable(L, -2);
	} break;
	default: {
		if (uvm::blockchain::is_any_table_storage_value_type(value.type)
			|| uvm::blockchain::is_any_array_storage_value_type(value.type))
		{
			lua_push_storage_table_value(L, value.value.table_value, value.type);
			break;
		}
		lua_pushnil(L);
	}
	}
	return true;
}

UvmStorageValue cbor_to_uvm_storage_value(lua_State *L, cbor::CborObject* cbor_value) {
	UvmStorageValue value;
	if (cbor_value->is_null())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_null;
		value.value.int_value = 0;
		return value;
	}
	else if (cbor_value->is_bool())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_bool;
		value.value.bool_value = cbor_value->as_bool();
		return value;
	}
	else if (cbor_value->is_int() || cbor_value->is_extra_int())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_int;
		value.value.int_value = cbor_value->force_as_int();
		return value;
	}
	else if (cbor_value->is_float())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_number;
		value.value.number_value = cbor_value->as_float64();
		return value;
	}
	else if (cbor_value->is_string())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_string;
		const auto& cbor_str = cbor_value->as_string();
		value.value.string_value = uvm::lua::lib::malloc_and_copy_string(L, cbor_str.c_str());
		return value;
	}
	else if (cbor_value->is_array())
	{
		const auto& cbor_array = cbor_value->as_array();
		value.value.table_value = uvm::lua::lib::create_managed_lua_table_map(L);
		if (cbor_array.empty())
		{
			value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_array;
		}
		else
		{
			std::vector<UvmStorageValue> item_values;
			for (size_t i = 0; i < cbor_array.size(); i++)
			{
				auto cbor_item = cbor_array[i];
				const auto &item_value = cbor_to_uvm_storage_value(L, cbor_item.get());
				item_values.push_back(item_value);
				(*value.value.table_value)[std::to_string(i + 1)] = item_value;
			}
			switch (item_values[0].type)
			{
			case uvm::blockchain::StorageValueTypes::storage_value_null:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_bool:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_bool_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_int:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_int_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_number:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_number_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_string:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_string_array;
				break;
			default:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_array;
			}
		}
		return value;
	}
	else if (cbor_value->is_map())
	{
		const auto& cbor_map = cbor_value->as_map();
		value.value.table_value = uvm::lua::lib::create_managed_lua_table_map(L);
		if (cbor_map.size()<1)
		{
			value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_table;
		}
		else
		{
			std::vector<UvmStorageValue> item_values;
			for (const auto &p : cbor_map)
			{
				const auto &item_value = cbor_to_uvm_storage_value(L, p.second.get());
				item_values.push_back(item_value);
				(*value.value.table_value)[p.first] = item_value;
			}
			switch (item_values[0].type)
			{
			case uvm::blockchain::StorageValueTypes::storage_value_null:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_bool:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_bool_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_int:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_int_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_number:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_number_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_string:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_string_table;
				break;
			default:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_table;
			}
		}
		return value;
	}
	else
	{
		throw cbor::CborException("not supported cbor value type");
	}
}
cbor::CborObjectP uvm_storage_value_to_cbor(UvmStorageValue value) {
	using namespace cbor;
	switch (value.type)
	{
	case uvm::blockchain::StorageValueTypes::storage_value_null:
		return CborObject::create_null();
	case uvm::blockchain::StorageValueTypes::storage_value_bool:
		return CborObject::from_bool(value.value.bool_value);
	case uvm::blockchain::StorageValueTypes::storage_value_int:
		return CborObject::from_int(value.value.int_value);
	case uvm::blockchain::StorageValueTypes::storage_value_number:
		return CborObject::from_float64(value.value.number_value);
	case uvm::blockchain::StorageValueTypes::storage_value_string:
		return CborObject::from_string(value.value.string_value);
	case uvm::blockchain::StorageValueTypes::storage_value_bool_array:
	case uvm::blockchain::StorageValueTypes::storage_value_int_array:
	case uvm::blockchain::StorageValueTypes::storage_value_number_array:
	case uvm::blockchain::StorageValueTypes::storage_value_string_array:
	case uvm::blockchain::StorageValueTypes::storage_value_unknown_array:
	{
		CborArrayValue cbor_array;
		for (const auto &p : *value.value.table_value)
		{
			cbor_array.push_back(uvm_storage_value_to_cbor(p.second));
		}
		return CborObject::create_array(cbor_array);
	}
	case uvm::blockchain::StorageValueTypes::storage_value_bool_table:
	case uvm::blockchain::StorageValueTypes::storage_value_int_table:
	case uvm::blockchain::StorageValueTypes::storage_value_number_table:
	case uvm::blockchain::StorageValueTypes::storage_value_string_table:
	case uvm::blockchain::StorageValueTypes::storage_value_unknown_table:
	{
		CborMapValue cbor_map;
		for (const auto &p : *value.value.table_value)
		{
			cbor_map[p.first] = uvm_storage_value_to_cbor(p.second);
		}
		return CborObject::create_map(cbor_map);
	}
	default:
		throw cbor::CborException("not supported cbor value type");
	}
}

jsondiff::JsonValue uvm_storage_value_to_json(UvmStorageValue value)
{
	switch (value.type)
	{
	case uvm::blockchain::StorageValueTypes::storage_value_null:
		return jsondiff::JsonValue();
	case uvm::blockchain::StorageValueTypes::storage_value_bool:
		return value.value.bool_value;
	case uvm::blockchain::StorageValueTypes::storage_value_int:
		return value.value.int_value;
	case uvm::blockchain::StorageValueTypes::storage_value_number:
		return value.value.number_value;
	case uvm::blockchain::StorageValueTypes::storage_value_string:
		return std::string(value.value.string_value);
	case uvm::blockchain::StorageValueTypes::storage_value_bool_array:
	case uvm::blockchain::StorageValueTypes::storage_value_int_array:
	case uvm::blockchain::StorageValueTypes::storage_value_number_array:
	case uvm::blockchain::StorageValueTypes::storage_value_string_array:
	case uvm::blockchain::StorageValueTypes::storage_value_unknown_array:
	{
		jsondiff::JsonArray json_array;
		for (const auto &p : *value.value.table_value)
		{
			json_array.push_back(uvm_storage_value_to_json(p.second));
		}
		return json_array;
	}
	case uvm::blockchain::StorageValueTypes::storage_value_bool_table:
	case uvm::blockchain::StorageValueTypes::storage_value_int_table:
	case uvm::blockchain::StorageValueTypes::storage_value_number_table:
	case uvm::blockchain::StorageValueTypes::storage_value_string_table:
	case uvm::blockchain::StorageValueTypes::storage_value_unknown_table:
	{
		jsondiff::JsonObject json_object;
		for (const auto &p : *value.value.table_value)
		{
			json_object[p.first] = uvm_storage_value_to_json(p.second);
		}
		return json_object;
	}
	default:
		throw jsondiff::JsonDiffException("not supported json value type");
	}
}

UvmStorageValue json_to_uvm_storage_value(lua_State *L, jsondiff::JsonValue json_value)
{
	UvmStorageValue value;
	if (json_value.is_null())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_null;
		value.value.int_value = 0;
		return value;
	}
	else if (json_value.is_bool())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_bool;
		value.value.bool_value = json_value.as_bool();
		return value;
	}
	else if (json_value.is_integer() || json_value.is_int64())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_int;
		value.value.int_value = json_value.as_int64();
		return value;
	}
	else if (json_value.is_numeric())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_number;
		value.value.number_value = json_value.as_double();
		return value;
	}
	else if (json_value.is_string())
	{
		value.type = uvm::blockchain::StorageValueTypes::storage_value_string;
		value.value.string_value = uvm::lua::lib::malloc_and_copy_string(L, json_value.as_string().c_str());
		return value;
	}
	else if (json_value.is_array())
	{
		jsondiff::JsonArray json_array = json_value.as<jsondiff::JsonArray>();
		value.value.table_value = uvm::lua::lib::create_managed_lua_table_map(L);
		if (json_array.empty())
		{
			value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_array;
		}
		else
		{
			std::vector<UvmStorageValue> item_values;
			for (size_t i = 0; i < json_array.size(); i++)
			{
				const auto &json_item = json_array[i];
				const auto &item_value = json_to_uvm_storage_value(L, json_item);
				item_values.push_back(item_value);
				(*value.value.table_value)[std::to_string(i + 1)] = item_value;
			}
			switch (item_values[0].type)
			{
			case uvm::blockchain::StorageValueTypes::storage_value_null:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_bool:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_bool_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_int:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_int_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_number:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_number_array;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_string:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_string_array;
				break;
			default:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_array;
			}
		}
		return value;
	}
	else if (json_value.is_object())
	{
		jsondiff::JsonObject json_map = json_value.as<jsondiff::JsonObject>();
		value.value.table_value = uvm::lua::lib::create_managed_lua_table_map(L);
		if (json_map.size()<1)
		{
			value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_table;
		}
		else
		{
			std::vector<UvmStorageValue> item_values;
			for (const auto &p : json_map)
			{
				const auto &item_value = json_to_uvm_storage_value(L, p.value());
				item_values.push_back(item_value);
				(*value.value.table_value)[p.key()] = item_value;
			}
			switch (item_values[0].type)
			{
			case uvm::blockchain::StorageValueTypes::storage_value_null:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_bool:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_bool_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_int:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_int_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_number:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_number_table;
				break;
			case uvm::blockchain::StorageValueTypes::storage_value_string:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_string_table;
				break;
			default:
				value.type = uvm::blockchain::StorageValueTypes::storage_value_unknown_table;
			}
		}
		return value;
	}
	else
	{
		throw jsondiff::JsonDiffException("not supported json value type");
	}
}

static UvmStorageChangeItem diff_storage_change_if_is_table(lua_State *L, UvmStorageChangeItem change_item, bool use_cbor_diff)
{
	if (!lua_storage_is_table(change_item.after.type))
		return change_item;
	if (!lua_storage_is_table(change_item.before.type) || !lua_storage_is_table(change_item.after.type))
		return change_item;
	try
	{
		if (use_cbor_diff) {
			const auto &before_cbor = uvm_storage_value_to_cbor(change_item.before);
			const auto &after_cbor = uvm_storage_value_to_cbor(change_item.after);
			cbor_diff::CborDiff differ;
			const auto &diff = differ.diff(before_cbor, after_cbor);
			change_item.cbor_diff = *diff;
		}
		else {
			const auto &before_json = uvm_storage_value_to_json(change_item.before);
			const auto &after_json = uvm_storage_value_to_json(change_item.after);
			jsondiff::JsonDiff json_diff;
			const auto &diff_json = json_diff.diff(before_json, after_json);
			change_item.diff = *diff_json;
		}
	}
	catch (jsondiff::JsonDiffException& e)
	{
		return change_item; // FIXME: throw error
	}
	catch (cbor_diff::CborDiffException& e) {
		return change_item;
	}
	catch (...) {
		return change_item;
	}
	return change_item;
}
static bool has_property_changed_in_changelist(UvmStorageChangeList *list, std::string contract_id, std::string name)
{
	if (nullptr == list)
		return false;
	for (auto it = list->begin(); it != list->end(); ++it)
	{
		auto item = *it;
		if (item.contract_id == contract_id && item.key == name)
			return true;
	}
	return false;
}


bool luaL_commit_storage_changes(lua_State *L)
{
	UvmStateValueNode storage_changelist_node = uvm::lua::lib::get_lua_state_value_node(L, LUA_STORAGE_CHANGELIST_KEY);
	if (global_uvm_chain_api->has_exception(L))
	{
		if (storage_changelist_node.type == LUA_STATE_VALUE_POINTER && nullptr != storage_changelist_node.value.pointer_value)
		{
			UvmStorageChangeList *list = (UvmStorageChangeList*)storage_changelist_node.value.pointer_value;
			list->clear();
		}
		return false;
	}
	auto use_cbor_diff = global_uvm_chain_api->use_cbor_diff(L);
	// merge changes
	std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, UvmStorageChangeItem>>> changes; // contract_id => (storage_unique_key => change_item)
	UvmStorageTableReadList *table_read_list = get_or_init_storage_table_read_list(L);
	if (storage_changelist_node.type != LUA_STATE_VALUE_POINTER && table_read_list)
	{
		storage_changelist_node.type = LUA_STATE_VALUE_POINTER;
		auto *list = (UvmStorageChangeList*)lua_malloc(L, sizeof(UvmStorageChangeList));
		if (!list)
		{
			return false;
		}
		new (list)UvmStorageChangeList();
		UvmStateValue value_to_store;
		value_to_store.pointer_value = list;
		uvm::lua::lib::set_lua_state_value(L, LUA_STORAGE_CHANGELIST_KEY, value_to_store, LUA_STATE_VALUE_POINTER);
		storage_changelist_node.value.pointer_value = list;
	}

	if (storage_changelist_node.type == LUA_STATE_VALUE_POINTER && nullptr != storage_changelist_node.value.pointer_value)
	{
		UvmStorageChangeList *list = (UvmStorageChangeList*)storage_changelist_node.value.pointer_value;
		
		bool mod_change_list = false;
		int64_t mod_change_list_fork_height = -1;
		if (global_uvm_chain_api) { //list->size()>0 ???
			mod_change_list_fork_height = global_uvm_chain_api->get_fork_height(L, "MOD_CHANGE_LIST");
			if (global_uvm_chain_api->get_header_block_num_without_gas(L) >= mod_change_list_fork_height) {
				mod_change_list = true;
				//fix bug, remove first null val
				if (list->size() > 0) {
					auto first_it = list->begin();
					if (first_it->after.type == uvm::blockchain::StorageValueTypes::storage_value_null&&
						first_it->before.type == uvm::blockchain::StorageValueTypes::storage_value_null) {
						list->pop_front();
					}
				}
			}
		}
		// merge initial tables here
		if (table_read_list)
		{
			for (auto it = table_read_list->begin(); it != table_read_list->end(); ++it)
			{
				auto change_item = *it;
				std::string global_skey = global_key_for_storage_prop(change_item.contract_id, change_item.key, change_item.fast_map_key, change_item.is_fast_map);
				lua_getglobal(L, global_skey.c_str());
				if (lua_istable(L, -1))
				{
					auto after_value = lua_type_to_storage_value_type(L, -1, 0);
					// check whether changelist has this property's change item. only read value if not exist
					//if (change_item.after.type != uvm::blockchain::StorageValueTypes::storage_value_null) {
						change_item.after = after_value;
					//}
					// FIXME: eg. a= {}, storage.a = a, a['name'] = 123, storage.a = {}   How to deal with the above circumstances?  maybe it will help to treat the storage as a table
					//if (!has_property_changed_in_changelist(list, change_item.contract_id, change_item.key))
					// {
					if (!change_item.before.equals(change_item.after))
					{
						list->push_back(change_item);
					}
					// }
				}
				lua_pop(L, 1);
			}
			table_read_list->clear();
		}
		std::set<std::string> null_keys_changed;
		for (auto it = list->begin(); it != list->end(); ++it)
		{
			UvmStorageChangeItem change_item = *it;
			const auto& change_item_full_key = change_item.full_key();
			if (!mod_change_list) {
				if (global_uvm_chain_api->use_fast_map_set_nil(L)) {
					if (change_item.is_fast_map && null_keys_changed.find(change_item_full_key) != null_keys_changed.end())
						continue;
				}
			}
			
			auto found = changes.find(change_item.contract_id);
			if (found != changes.end())
			{
				auto contract_changes = found->second;

				auto found_key = contract_changes->find(change_item_full_key);
				if (found_key != contract_changes->end())
				{
					found_key->second.after = change_item.after;
				}
				else
				{
					contract_changes->insert(std::make_pair(change_item_full_key, change_item));
				}
			}
			else
			{
				auto contract_changes = std::make_shared<std::unordered_map<std::string, UvmStorageChangeItem>>();
				contract_changes->insert(contract_changes->end(), std::make_pair(change_item_full_key, change_item));
				changes.insert(changes.end(), std::make_pair(change_item.contract_id, contract_changes));
			}
			if (!mod_change_list) {  //if mod_change_list == true ,don't insert to null_keys_changed
				if (change_item.after.type == uvm::blockchain::StorageValueTypes::storage_value_null)
					null_keys_changed.insert(change_item_full_key);
			}
		}
	}
	else
	{
		return false;
	}
	if (uvm::lua::lib::is_calling_contract_init_api(L)
		&& changes.size() == 0)
	{
		auto starting_contract_address = uvm::lua::lib::get_starting_contract_address(L);
		auto stream = global_uvm_chain_api->open_contract_by_address(L, starting_contract_address.c_str());
		if (stream && stream->contract_storage_properties.size() > 0)
		{
			global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "some storage of this contract not init");
			return false;
		}
	}
	for (auto it = changes.begin(); it != changes.end(); ++it)
	{
		auto stream = global_uvm_chain_api->open_contract_by_address(L, it->first.c_str());
		if (!stream)
		{
			global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "Can't get contract info by contract address %s", it->first.c_str());
			return false;
		}
		bool is_in_starting_contract_init = false;
		if (uvm::lua::lib::is_calling_contract_init_api(L))
		{
			auto starting_contract_address = uvm::lua::lib::get_starting_contract_address(L);
			if (it->first == starting_contract_address)
			{
				is_in_starting_contract_init = true;
				const auto &storage_properties_in_chain = stream->contract_storage_properties;
				/*if (it->second->size() != storage_properties_in_chain.size())
				{
					global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "some storage of this contract not init");
					return false;
				}*/
				for (auto &p1 : *(it->second))
				{
					if (p1.second.is_fast_map)
					{
						continue;
					}
					if (storage_properties_in_chain.find(p1.second.key) == storage_properties_in_chain.end())
					{
						global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "Can't find storage %s", p1.second.key.c_str());
						return false;
					}
					auto storage_info_in_chain = storage_properties_in_chain.at(p1.second.key);
					if (uvm::blockchain::is_any_table_storage_value_type(p1.second.after.type)
						|| uvm::blockchain::is_any_array_storage_value_type(p1.second.after.type))
					{
						// [] will show as {} in runtime
						if (!uvm::blockchain::is_any_table_storage_value_type(storage_info_in_chain)
							&& !uvm::blockchain::is_any_array_storage_value_type(storage_info_in_chain))
						{
							global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage %s type not matched in chain", p1.second.key.c_str());
							return false;
						}
						if (p1.second.after.value.table_value->size()>0)
						{
							for (auto &item_in_table : *(p1.second.after.value.table_value))
							{
								item_in_table.second.try_parse_type(uvm::blockchain::get_storage_base_type(storage_info_in_chain));
							}
							auto item_after = p1.second.after.value.table_value->begin()->second;
							if (item_after.type != uvm::blockchain::get_item_type_in_table_or_array(storage_info_in_chain))
							{
								global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage %s type not matched in chain", p1.second.key.c_str());
								return false;
							}
						}
						if (p1.second.after.type == uvm::blockchain::storage_value_unknown_table
							|| p1.second.after.type == uvm::blockchain::storage_value_unknown_array)
						{
							p1.second.after.type = storage_info_in_chain;
						}
					}
					else if (p1.second.after.type == uvm::blockchain::StorageValueTypes::storage_value_number
						&& storage_info_in_chain == uvm::blockchain::StorageValueTypes::storage_value_int)
					{
						p1.second.after.type = uvm::blockchain::StorageValueTypes::storage_value_int;
						p1.second.after.value.int_value = (lua_Integer)p1.second.after.value.number_value;
					}
					else if (p1.second.after.type == uvm::blockchain::StorageValueTypes::storage_value_int
						&& storage_info_in_chain == uvm::blockchain::StorageValueTypes::storage_value_number)
					{
						p1.second.after.type = uvm::blockchain::StorageValueTypes::storage_value_number;
						p1.second.after.value.number_value = (lua_Number)p1.second.after.value.int_value;
					}
				}
			}
		}
		auto it2 = it->second->begin();
		while (it2 != it->second->end())
		{
			if (lua_storage_is_table(it2->second.after.type))
			{
				// when before is empty table, and after is array
				// when before is array, and after is empty table
				if (lua_storage_is_array(it2->second.before.type) && it2->second.after.value.table_value->size() == 0)
					it2->second.after.type = it2->second.before.type;
				else if (lua_storage_is_table(it2->second.before.type) && it2->second.before.value.table_value->size()>0)
					it2->second.after.type = it2->second.before.type;
				// just save table diff
				it2->second = diff_storage_change_if_is_table(L, it2->second, use_cbor_diff);
			}
			// check storage changes and the corresponding types of compile-time contracts, and modify the type of commit
			if (!is_in_starting_contract_init)
			{
				const auto &storage_properties_in_chain = stream->contract_storage_properties;
				if (it2->second.before.type != uvm::blockchain::StorageValueTypes::storage_value_null
					&& storage_properties_in_chain.find(it2->first) != storage_properties_in_chain.end())
				{
					auto storage_info_in_chain = storage_properties_in_chain.at(it2->first);
					if (uvm::blockchain::is_any_table_storage_value_type(it2->second.after.type)
						|| uvm::blockchain::is_any_array_storage_value_type(it2->second.after.type))
					{
						// [] showed as {} in runtime
						if (!uvm::blockchain::is_any_table_storage_value_type(storage_info_in_chain)
							&& !uvm::blockchain::is_any_array_storage_value_type(storage_info_in_chain))
						{
							global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage %s type not matched in chain", it2->first.c_str());
							return false;
						}
						if (it2->second.after.value.table_value->size() > 0)
						{
							for (auto &item_in_table : *(it2->second.after.value.table_value))
							{
								item_in_table.second.try_parse_type(uvm::blockchain::get_storage_base_type(storage_info_in_chain));
							}
							auto item_after = it2->second.after.value.table_value->begin()->second;
							if (item_after.type != uvm::blockchain::get_item_type_in_table_or_array(storage_info_in_chain))
							{
								global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage %s type not matched in chain", it2->first.c_str());
								return false;
							}
						}
						// check match of after's value type and type in chain
						if (it2->second.after.type == uvm::blockchain::storage_value_unknown_table
							|| it2->second.after.type == uvm::blockchain::storage_value_unknown_array)
						{
							it2->second.after.type = storage_info_in_chain;
						}
					}
					else if (it2->second.after.type == uvm::blockchain::StorageValueTypes::storage_value_number
						&& storage_info_in_chain == uvm::blockchain::StorageValueTypes::storage_value_int)
					{
						it2->second.after.type = uvm::blockchain::StorageValueTypes::storage_value_int;
						it2->second.after.value.int_value = (lua_Integer)it2->second.after.value.number_value;
					}
					else if (it2->second.after.type == uvm::blockchain::StorageValueTypes::storage_value_int
						&& storage_info_in_chain == uvm::blockchain::StorageValueTypes::storage_value_number)
					{
						it2->second.after.type = uvm::blockchain::StorageValueTypes::storage_value_number;
						it2->second.after.value.number_value = (lua_Number)it2->second.after.value.int_value;
					}
				}
			}
			// map/array's sub item values' types must be consistent, and are primitive types
			if (lua_storage_is_table(it2->second.after.type))
			{
				uvm::blockchain::StorageValueTypes item_value_type;
				bool is_first = true;
				for (const auto &p : *it2->second.after.value.table_value)
				{
					if (is_first)
					{
						item_value_type = p.second.type;
						continue;
					}
					is_first = true;
					if (!UvmStorageValue::is_same_base_type_with_type_parse(p.second.type, item_value_type))
					{
						global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
							"array/map's value type must be same in contract storage");
						return false;
					}
				}
			}
			if (it2->second.after.type == it2->second.before.type)
			{
				if (it2->second.after.type == uvm::blockchain::StorageValueTypes::storage_value_int
					&& it2->second.after.value.int_value == it2->second.before.value.int_value)
				{
					it2 = it->second->erase(it2);
					continue;
				}
				else if (it2->second.after.type == uvm::blockchain::StorageValueTypes::storage_value_bool
					&& it2->second.after.value.bool_value == it2->second.before.value.bool_value)
				{
					it2 = it->second->erase(it2);
					continue;
				}
				else if (it2->second.after.type == uvm::blockchain::StorageValueTypes::storage_value_number
					&& abs(it2->second.after.value.number_value - it2->second.before.value.number_value) < 0.0000001)
				{
					it2 = it->second->erase(it2);
					continue;
				}
				else if (it2->second.after.type == uvm::blockchain::StorageValueTypes::storage_value_string
					&& strcmp(it2->second.after.value.string_value, it2->second.before.value.string_value) == 0)
				{
					it2 = it->second->erase(it2);
					continue;
				}
				else if (it2->second.after.type == uvm::blockchain::StorageValueTypes::storage_value_stream
					&& ((uvm::lua::lib::UvmByteStream*) it2->second.after.value.userdata_value)->equals((uvm::lua::lib::UvmByteStream*) it2->second.before.value.userdata_value))
				{
					it2 = it->second->erase(it2);
					continue;
				}
				else if (!lua_storage_is_table(it2->second.after.type)
					&& it2->second.after.type != uvm::blockchain::StorageValueTypes::storage_value_string
					&& memcmp(&it2->second.after.value, &it2->second.before.value, sizeof(UvmStorageValueUnion)) == 0)
				{
					it2 = it->second->erase(it2);
					continue;
				}
				else if (lua_storage_is_table(it2->second.after.type))
				{
					if ((it2->second.after.value.table_value->size() == 0
						&& it2->second.before.value.table_value->size() == 0))
					{
						it2 = it->second->erase(it2);
						continue;
					}
				}
			}
			++it2;
		}
	}

	// commit changes to uvm
	if (uvm::lua::lib::check_in_lua_sandbox(L))
	{
		printf("commit storage changes in sandbox\n");
		return false;
	}
	auto result = global_uvm_chain_api->commit_storage_changes_to_uvm(L, changes);
	if (storage_changelist_node.type == LUA_STATE_VALUE_POINTER && nullptr != storage_changelist_node.value.pointer_value)
	{
		UvmStorageChangeList *list = (UvmStorageChangeList*)storage_changelist_node.value.pointer_value;
		list->clear();
	}
	return result;
}


namespace uvm {
	namespace lib {

		int uvmlib_get_storage_impl(lua_State *L,
			const char *contract_id, const char *name, const char* fast_map_key, bool is_fast_map)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);

			const auto &code_storage_contract_id = get_contract_id_string_in_storage_operation(L);
			/*if (code_storage_contract_id != contract_id)
			{
				global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "contract can only access its own storage directly");
				uvm::lua::lib::notify_lua_state_stop(L);
				L->force_stopping = true;
				return 0;
			}*/
			contract_id = code_storage_contract_id.c_str(); // storage改成只用当前所在合约
			std::string fast_map_key_str = fast_map_key ? fast_map_key : "";
			const auto &global_key = global_key_for_storage_prop(contract_id, name, fast_map_key_str, is_fast_map);
			lua_getglobal(L, global_key.c_str());
			if (lua_istable(L, -1))
			{
				// auto value = lua_type_to_storage_value_type(L, -1, 0);
				return 1;
			}
			lua_pop(L, 1);
			const auto &state_value_node = uvm::lua::lib::get_lua_state_value_node(L, LUA_STORAGE_CHANGELIST_KEY);
			int result;
			if (state_value_node.type != LUA_STATE_VALUE_POINTER || !state_value_node.value.pointer_value)
			{
				const auto &value = get_last_storage_changed_value(L, contract_id, nullptr, std::string(name), fast_map_key ? std::string(fast_map_key) : std::string(""), is_fast_map);
				lua_push_storage_value(L, value);
				if (lua_storage_is_table(value.type))
				{
					lua_pushvalue(L, -1);
					lua_setglobal(L, global_key.c_str());
					// uvm::lua::lib::add_maybe_storage_changed_contract_id(L, contract_id);
				}
				result = 1;
			}
			else if (uvm::lua::lib::check_in_lua_sandbox(L))
			{
				printf("get storage in sandbox\n");
				lua_pushnil(L);
				result = 1;
			}
			else
			{
				UvmStorageChangeList *list = (UvmStorageChangeList*)state_value_node.value.pointer_value;
				const auto &value = get_last_storage_changed_value(L, contract_id, list, std::string(name), fast_map_key ? std::string(fast_map_key) : std::string(""), is_fast_map);
				lua_push_storage_value(L, value);
				if (lua_storage_is_table(value.type))
				{
					lua_pushvalue(L, -1);
					lua_setglobal(L, global_key_for_storage_prop(contract_id, name, fast_map_key_str, is_fast_map).c_str());
				}
				result = 1;
			}
			return result;
		}

		int uvmlib_set_storage_impl(lua_State *L,
			const char *contract_id, const char *name, const char* fast_map_key, bool is_fast_map, int value_index)
		{
			const auto &code_storage_contract_id = get_contract_id_string_in_storage_operation(L);
			/*if (code_storage_contract_id != contract_id)
			{
				global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "contract can only access its own storage directly");
				uvm::lua::lib::notify_lua_state_stop(L);
				L->force_stopping = true;
				return 0;
			}*/
			contract_id = code_storage_contract_id.c_str(); // storage改成只用当前所在合约
			auto contract_id_stack = uvm::lua::lib::get_using_contract_id_stack(L, true);
			if (contract_id_stack && contract_id_stack->size()>0 && contract_id_stack->top().call_type == "STATIC_CALL") {
				global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "static call can not modify contract storage");
				uvm::lua::lib::notify_lua_state_stop(L);
				L->force_stopping = true;
				return 0;
			}
			if (!name || strlen(name) < 1)
			{
				global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "second argument of set_storage must be name");
				return 0;
			}
			std::string fast_map_key_str = fast_map_key ? fast_map_key : "";

			// uvm::lua::lib::add_maybe_storage_changed_contract_id(L, contract_id);

			// FIXME: If this is a table, each time you create a new object, take up too much memory, and read too slow
			// FIXME: When considering the commit to read storage changes, do not change every time
			const auto &arg2 = lua_type_to_storage_value_type(L, value_index, 0);
			if (lua_istable(L, value_index))
			{
				// add it to read_list if it's table, because it will be changed
				lua_pushvalue(L, value_index);
				lua_setglobal(L, global_key_for_storage_prop(contract_id, name, fast_map_key_str, is_fast_map).c_str());
				auto *table_read_list = get_or_init_storage_table_read_list(L);
				if (table_read_list)
				{
					bool found = false;
					for (auto it = table_read_list->begin(); it != table_read_list->end(); ++it)
					{
						if (it->contract_id == std::string(contract_id) && it->key == name && it->fast_map_key== fast_map_key_str && it->is_fast_map==is_fast_map)
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						UvmStorageChangeItem change_item;
						change_item.contract_id = contract_id;
						change_item.key = name;
						change_item.fast_map_key = fast_map_key_str;
						change_item.is_fast_map = is_fast_map;
						change_item.before = arg2;
						change_item.after = arg2;
						// merge change history to release too many not-used-again objects
						table_read_list->push_back(change_item);
					}
				}
			}
			/*
			if (arg2.type >= LVALUE_NOT_SUPPORT)
			{
			global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "third argument of set_storage must be value");
			return 0;
			}
			*/

			// log the value before and the new value
			UvmStateValueNode state_value_node = uvm::lua::lib::get_lua_state_value_node(L, LUA_STORAGE_CHANGELIST_KEY);
			UvmStorageChangeList *list;
			if (state_value_node.type != LUA_STATE_VALUE_POINTER || nullptr == state_value_node.value.pointer_value)
			{
				list = (UvmStorageChangeList*)lua_malloc(L, sizeof(UvmStorageChangeList));
				if (!list)
				{
					return 0;
				}
				new (list)UvmStorageChangeList();
				UvmStateValue value_to_store;
				value_to_store.pointer_value = list;
				uvm::lua::lib::set_lua_state_value(L, LUA_STORAGE_CHANGELIST_KEY, value_to_store, LUA_STATE_VALUE_POINTER);
			}
			else
			{
				list = (UvmStorageChangeList*)state_value_node.value.pointer_value;
			}
			if (uvm::lua::lib::check_in_lua_sandbox(L))
			{
				printf("set storage in sandbox\n");
				return 0;
			}
			std::string name_str(name);
			auto before = get_last_storage_changed_value(L, contract_id, list, name_str, fast_map_key_str, is_fast_map);
			auto after = arg2;
			if (!is_fast_map && after.type == uvm::blockchain::StorageValueTypes::storage_value_null)
			{
				global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, (name_str + " storage can't change to nil").c_str());
				uvm::lua::lib::notify_lua_state_stop(L);
				return 0;
			}
			if (!is_fast_map && (before.type != uvm::blockchain::StorageValueTypes::storage_value_null
				&& (before.type != after.type && !lua_storage_is_table(before.type))))
			{
				global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, (std::string(name) + " storage can't change type").c_str());
				uvm::lua::lib::notify_lua_state_stop(L);
				return 0;
			}

			if (is_fast_map && (after.type == uvm::blockchain::StorageValueTypes::storage_value_null)
				&& (lua_storage_is_table(before.type))) {
				if (global_uvm_chain_api) { 
					int64_t mod_change_list_fork_height = global_uvm_chain_api->get_fork_height(L, "MOD_CHANGE_LIST");
					if (global_uvm_chain_api->get_header_block_num_without_gas(L) >= mod_change_list_fork_height) {
						// has been added to table_read_list when get_last_storage_changed_value
						//set val
						lua_pushvalue(L, value_index);
						lua_setglobal(L, global_key_for_storage_prop(contract_id, name, fast_map_key_str, is_fast_map).c_str());
					}
				}

			}

			// can't create new storage index outside init
			if (!uvm::lua::lib::is_calling_contract_init_api(L)
				&& before.type == uvm::blockchain::StorageValueTypes::storage_value_null)
			{
				if (!is_fast_map) {
					// when not in init api
					global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, (std::string(name) + "storage can't register storage after inited").c_str());
					uvm::lua::lib::notify_lua_state_stop(L);
					return 0;
				}
			}

			// disable nested map
			if (lua_storage_is_table(after.type))
			{
				auto inner_table = after.value.table_value;
				if (nullptr != inner_table)
				{
					for (auto it = inner_table->begin(); it != inner_table->end(); ++it)
					{
						if (lua_storage_is_table(it->second.type))
						{
							global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage not support nested map");
							uvm::lua::lib::notify_lua_state_stop(L);
							return 0;
						}
					}
				}
			}

			if (lua_storage_is_table(after.type))
			{
				// table properties can't change type except change to nil(in whole history in this lua_State)
				// value type must be same in table in storage
				int table_value_type = -1;
				if (lua_storage_is_table(before.type))
				{
					for (auto it = before.value.table_value->begin(); it != before.value.table_value->end(); ++it)
					{
						auto found = after.value.table_value->find(it->first);
						if (it->second.type != uvm::blockchain::StorageValueTypes::storage_value_null)
						{
							if (table_value_type < 0)
							{
								table_value_type = it->second.type;
							}
							else
							{
								if (table_value_type != it->second.type)
								{
									global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage table type must be same");
									uvm::lua::lib::notify_lua_state_stop(L);
									return 0;
								}
							}
						}
					}
				}
				for (auto it = after.value.table_value->begin(); it != after.value.table_value->end(); ++it)
				{
					if (it->second.type != uvm::blockchain::StorageValueTypes::storage_value_null)
					{
						if (table_value_type < 0)
						{
							table_value_type = it->second.type;
						}
						else
						{
							if (table_value_type != it->second.type)
							{
								global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage table type must be same");
								uvm::lua::lib::notify_lua_state_stop(L);
								return 0;
							}
						}
					}
				}
				if ((!lua_storage_is_table(before.type) || before.value.table_value->size() < 1) && after.value.table_value->size() > 0)
				{
					// if before table is empty and after table not empty, search type before
					for (auto it = list->begin(); it != list->end(); ++it)
					{
						if (it->contract_id == std::string(contract_id) && it->key == std::string(name) && it->fast_map_key==fast_map_key_str && it->is_fast_map==is_fast_map)
						{
							if (lua_storage_is_table(it->after.type) && it->after.value.table_value->size() > 0)
							{
								for (auto it2 = it->after.value.table_value->begin(); it2 != it->after.value.table_value->end(); ++it2)
								{
									if (it2->second.type != uvm::blockchain::StorageValueTypes::storage_value_null)
									{
										if (it2->second.type != table_value_type)
										{
											global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "storage table type must be same");
											uvm::lua::lib::notify_lua_state_stop(L);
											return 0;
										}
									}
								}
							}
						}
					}
				}
			}


			UvmStorageChangeItem change_item;
			change_item.key = name;
			change_item.contract_id = contract_id;
			change_item.after = after;
			change_item.before = before;
			change_item.fast_map_key = fast_map_key;
			change_item.is_fast_map = is_fast_map;
			// merge change history to release too many not-used-again objects
			list->push_back(change_item);

			return 0;
		}

	}
}
