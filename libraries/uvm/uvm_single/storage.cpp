#include <simplechain/storage.h>
#include <fc/io/json.hpp>
#include <uvm/uvm_lib.h>
#include <map>

namespace simplechain {
	using namespace uvm::blockchain;

	fc::variant uvm_storage_value_to_json(UvmStorageValue value)
	{
		switch (value.type)
		{
		case uvm::blockchain::StorageValueTypes::storage_value_null:
			return fc::variant();
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
			fc::variants json_array;
			for (const auto &p : *value.value.table_value)
			{
				json_array.push_back(simplechain::uvm_storage_value_to_json(p.second));
			}
			return json_array;
		}
		case uvm::blockchain::StorageValueTypes::storage_value_bool_table:
		case uvm::blockchain::StorageValueTypes::storage_value_int_table:
		case uvm::blockchain::StorageValueTypes::storage_value_number_table:
		case uvm::blockchain::StorageValueTypes::storage_value_string_table:
		case uvm::blockchain::StorageValueTypes::storage_value_unknown_table:
		{
			fc::mutable_variant_object json_object;
			for (const auto &p : *value.value.table_value)
			{
				json_object[p.first] = simplechain::uvm_storage_value_to_json(p.second);
			}
			return json_object;
		}
		default:
			throw uvm::core::UvmException("not supported json value type");
		}
	}


	std::map <::uvm::blockchain::StorageValueTypes, std::string> storage_type_map = \
	{
		make_pair(storage_value_null, std::string("nil")),
			make_pair(storage_value_int, std::string("int")),
			make_pair(storage_value_number, std::string("number")),
			make_pair(storage_value_bool, std::string("bool")),
			make_pair(storage_value_string, std::string("string")),
			make_pair(storage_value_unknown_table, std::string("Map<unknown type>")),
			make_pair(storage_value_int_table, std::string("Map<int>")),
			make_pair(storage_value_number_table, std::string("Map<number>")),
			make_pair(storage_value_bool_table, std::string("Map<bool>")),
			make_pair(storage_value_string_table, std::string("Map<string>")),
			make_pair(storage_value_unknown_array, std::string("Array<unknown type>")),
			make_pair(storage_value_int_array, std::string("Array<int>")),
			make_pair(storage_value_number_array, std::string("Array<number>")),
			make_pair(storage_value_bool_array, std::string("Array<bool>")),
			make_pair(storage_value_string_array, std::string("Array<string>"))
	};
	const uint8_t StorageNullType::type = storage_value_null;
	const uint8_t StorageIntType::type = storage_value_int;
	const uint8_t StorageNumberType::type = storage_value_number;
	const uint8_t StorageBoolType::type = storage_value_bool;
	const uint8_t StorageStringType::type = storage_value_string;
	const uint8_t StorageIntTableType::type = storage_value_int_table;
	const uint8_t StorageNumberTableType::type = storage_value_number_table;
	const uint8_t StorageBoolTableType::type = storage_value_bool_table;
	const uint8_t StorageStringTableType::type = storage_value_string_table;
	const uint8_t StorageIntArrayType::type = storage_value_int_array;
	const uint8_t StorageNumberArrayType::type = storage_value_number_array;
	const uint8_t StorageBoolArrayType::type = storage_value_bool_array;
	const uint8_t StorageStringArrayType::type = storage_value_string_array;

	StorageDataType StorageDataType::get_storage_data_from_lua_storage(const UvmStorageValue& lua_storage)
	{
		auto storage_json = simplechain::uvm_storage_value_to_json(lua_storage);
		StorageDataType storage_data(fc::json::to_string(storage_json));
		return storage_data;
	}

	static fc::variant json_from_chars(std::vector<char> data_chars)
	{
		std::vector<char> data(data_chars.size() + 1);
		memcpy(data.data(), data_chars.data(), sizeof(char) * data_chars.size());
		data[data_chars.size()] = '\0';
		std::string storage_json_str(data.data());
		return fc::json::from_string(storage_json_str);
	}

	static fc::variant json_from_str(const std::string &str)
	{
		return fc::json::from_string(str);
	}


	UvmStorageValue json_to_uvm_storage_value(lua_State *L, fc::variant json_value)
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
			fc::variants json_array = json_value.as<fc::variants>();
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
					const auto &item_value = simplechain::json_to_uvm_storage_value(L, json_item);
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
			fc::mutable_variant_object json_map = json_value.as<fc::mutable_variant_object>();
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
					const auto &item_value = simplechain::json_to_uvm_storage_value(L, p.value());
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
			throw uvm::core::UvmException("not supported json value type");
		}
	}

	UvmStorageValue StorageDataType::create_lua_storage_from_storage_data(lua_State *L, const StorageDataType& storage)
	{
		auto json_value = json_from_str(storage.as<std::string>());
		auto value = simplechain::json_to_uvm_storage_value(L, json_value);
		return value;
	}

}

namespace fc {
	void to_variant(const simplechain::StorageDataType& var, variant& vo) {
		fc::mutable_variant_object obj("storage_data", var.storage_data);
		vo = std::move(obj);
	}
}
