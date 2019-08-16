#pragma once
#include <simplechain/config.h>
#include <simplechain/contract_entry.h>
#include <vector>

namespace simplechain {

	struct contract_object {
		uint64_t registered_block = 0;
		uvm::blockchain::Code code;
		std::string owner_address;
		fc::time_point_sec create_time;
		std::string contract_address;
		std::string contract_name;
		std::string contract_desc;
		contract_type type_of_contract = normal_contract;
		std::string native_contract_key; // key to find native contract code
		std::vector<std::string> derived;
		std::string inherit_from;

		static ContractEntryPrintable to_printable(const contract_object& obj);
	};
}

namespace fc {
	void to_variant(const simplechain::contract_object& var, variant& vo);
	void from_variant(const fc::variant& var, simplechain::contract_object& vo);
}

// FC_REFLECT(simplechain::contract_object, (registered_block)(code)(owner_address)(create_time)(contract_address)(contract_name)(contract_desc)(type_of_contract)(native_contract_key)(derived)(inherit_from))
