#pragma once
#include <simplechain/config.h>

namespace simplechain {

	struct StorageNullType
	{
		StorageNullType() : raw_storage(0) {}
		static const uint8_t    type;
		LUA_INTEGER raw_storage;
	};
	struct StorageIntType
	{
		StorageIntType() {}
		StorageIntType(LUA_INTEGER value) :raw_storage(value) {}
		static const uint8_t    type;
		LUA_INTEGER raw_storage;
	};
	struct StorageNumberType
	{
		StorageNumberType() {}
		StorageNumberType(double value) :raw_storage(value) {}
		static const uint8_t    type;
		double raw_storage;
	};
	struct StorageBoolType
	{
		StorageBoolType() {}
		StorageBoolType(bool value) :raw_storage(value) {}
		static const uint8_t    type;
		bool raw_storage;
	};
	struct StorageStringType
	{
		StorageStringType() {}
		StorageStringType(std::string value) :raw_storage(value) {}
		static const uint8_t    type;
		std::string raw_storage;
	};
	//table
	struct StorageIntTableType
	{
		StorageIntTableType() {}
		StorageIntTableType(const std::map<std::string, LUA_INTEGER>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, LUA_INTEGER> raw_storage_map;
	};
	struct StorageNumberTableType
	{
		StorageNumberTableType() {}
		StorageNumberTableType(const std::map<std::string, double>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, double> raw_storage_map;
	};
	struct StorageBoolTableType
	{
		StorageBoolTableType() {}
		StorageBoolTableType(const std::map<std::string, bool>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, bool> raw_storage_map;
	};
	struct StorageStringTableType
	{
		StorageStringTableType() {}
		StorageStringTableType(const std::map<std::string, std::string>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, std::string> raw_storage_map;
	};
	//array
	struct StorageIntArrayType
	{
		StorageIntArrayType() {}
		StorageIntArrayType(const std::map<std::string, LUA_INTEGER>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, LUA_INTEGER> raw_storage_map;
	};
	struct StorageNumberArrayType
	{
		StorageNumberArrayType() {}
		StorageNumberArrayType(const std::map<std::string, double>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, double> raw_storage_map;
	};
	struct StorageBoolArrayType
	{
		StorageBoolArrayType() {}
		StorageBoolArrayType(const std::map<std::string, bool>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, bool> raw_storage_map;
	};
	struct StorageStringArrayType
	{
		StorageStringArrayType() {}
		StorageStringArrayType(const std::map<std::string, std::string>& value_map) :raw_storage_map(value_map) {}
		static const uint8_t    type;
		std::map<std::string, std::string> raw_storage_map;
	};


	struct StorageDataType
	{
		std::vector<char> storage_data;
		StorageDataType() {}
		template<typename StorageType>
		StorageDataType(const StorageType& t)
		{
			storage_data = fc::raw::pack(t);
		}
		template<typename StorageType>
		StorageType as()const
		{
			return fc::raw::unpack<StorageType>(storage_data);
		}
		static StorageDataType get_storage_data_from_lua_storage(const UvmStorageValue& lua_storage);
		static UvmStorageValue create_lua_storage_from_storage_data(lua_State *L, const StorageDataType& storage_data);
	};

	struct comparator_for_string {
		inline bool operator() (const std::string& x, const std::string& y) const
		{
			return x < y;
		}
	};

	struct StorageDataChangeType
	{
		StorageDataType storage_diff;
		cbor_diff::DiffResult cbor_diff;
		StorageDataType before;
		StorageDataType after;
	};

	typedef std::map<std::string, StorageDataChangeType, comparator_for_string> contract_storage_changes_type;

	fc::variant uvm_storage_value_to_json(UvmStorageValue value);
	UvmStorageValue json_to_uvm_storage_value(lua_State *L, fc::variant json_value);

}

namespace fc {
	void to_variant(const simplechain::StorageDataType& var, variant& vo);
}

FC_REFLECT(simplechain::StorageDataType,
(storage_data)
)
FC_REFLECT(simplechain::StorageNullType,
(raw_storage)
)
FC_REFLECT(simplechain::StorageIntType,
(raw_storage)
)
FC_REFLECT(simplechain::StorageBoolType,
(raw_storage)
)
FC_REFLECT(simplechain::StorageNumberType,
(raw_storage)
)
FC_REFLECT(simplechain::StorageStringType,
(raw_storage)
)
FC_REFLECT(simplechain::StorageIntTableType,
(raw_storage_map)
)
FC_REFLECT(simplechain::StorageBoolTableType,
(raw_storage_map)
)
FC_REFLECT(simplechain::StorageNumberTableType,
(raw_storage_map)
)
FC_REFLECT(simplechain::StorageStringTableType,
(raw_storage_map)
)
FC_REFLECT(simplechain::StorageIntArrayType,
(raw_storage_map)
)
FC_REFLECT(simplechain::StorageBoolArrayType,
(raw_storage_map)
)
FC_REFLECT(simplechain::StorageNumberArrayType,
(raw_storage_map)
)
FC_REFLECT(simplechain::StorageStringArrayType,
(raw_storage_map)
)

FC_REFLECT(simplechain::StorageDataChangeType, (storage_diff)(before)(after))
