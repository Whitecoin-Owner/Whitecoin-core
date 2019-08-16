#pragma once
#include <simplechain/config.h>
#include <fc/reflect/reflect.hpp>
#include <uvm/uvm_api.h>

namespace uvm {
	namespace blockchain {
		struct Code
		{
			std::set<std::string> abi;
			std::set<std::string> offline_abi;
			std::set<std::string> events;
			std::map<std::string, fc::enum_type<fc::unsigned_int, uvm::blockchain::StorageValueTypes>> storage_properties;

			std::map<std::string, std::vector<UvmTypeInfoEnum>> api_arg_types;
			std::vector<unsigned char> code;
			std::string code_hash;
			Code() {}
			void SetApis(char* module_apis[], int count, int api_type);
			bool valid() const;
			std::string GetHash() const;
			bool operator!=(const Code& it)const;
		};
	}
}

namespace simplechain {

	enum contract_type
	{
		normal_contract,
		native_contract,
		contract_based_on_template
	};

	enum ContractApiType
	{
		chain = 1,
		offline = 2,
		event = 3
	};

	struct CodePrintAble
	{
		std::set<std::string> abi;
		std::set<std::string> offline_abi;
		std::set<std::string> events;
		std::map<std::string, std::string> printable_storage_properties;
		std::string printable_code;
		std::string code_hash;

		CodePrintAble() {}

		CodePrintAble(const uvm::blockchain::Code& code);
	};

	struct ContractEntryPrintable
	{
		ContractEntryPrintable() {}
		std::string  id; //contract address
		std::string owner_address;  //the owner address of the contract
		std::string owner_name;  //the owner name of the contract
		std::string name;
		std::string description;
		contract_type type_of_contract;
		uint32_t registered_block;
		fc::optional<std::string> inherit_from;

		std::vector<std::string> derived;
		CodePrintAble code_printable; // code-related of contract
		fc::time_point_sec createtime;
	};

	typedef uint64_t gas_price_type;
	typedef uint64_t gas_count_type;

}

namespace fc {
	void from_variant(const fc::variant& var, simplechain::contract_type& vo);
	void from_variant(const fc::variant& var, std::vector<unsigned char>& vo);
	void from_variant(const fc::variant& var, fc::enum_type<fc::unsigned_int, uvm::blockchain::StorageValueTypes>& vo);
	void from_variant(const fc::variant& var, uvm::blockchain::Code& vo);
}

FC_REFLECT_ENUM(simplechain::contract_type, (normal_contract)(native_contract)(contract_based_on_template))
FC_REFLECT_ENUM(simplechain::ContractApiType, (chain)(offline)(event))


FC_REFLECT_ENUM(uvm::blockchain::StorageValueTypes,
(storage_value_null)
(storage_value_int)
(storage_value_number)
(storage_value_bool)
(storage_value_string)
(storage_value_unknown_table)
(storage_value_int_table)
(storage_value_number_table)
(storage_value_bool_table)
(storage_value_string_table)
(storage_value_unknown_array)
(storage_value_int_array)
(storage_value_number_array)
(storage_value_bool_array)
(storage_value_string_array)
)

FC_REFLECT(uvm::blockchain::Code, (abi)(offline_abi)(events)(storage_properties)(code)(code_hash))

FC_REFLECT(simplechain::CodePrintAble, (abi)(offline_abi)(events)(printable_storage_properties)(printable_code)(code_hash));
FC_REFLECT(simplechain::ContractEntryPrintable, (id)(owner_address)(owner_name)(name)(description)(type_of_contract)(registered_block)(inherit_from)(derived)(code_printable)(createtime));

