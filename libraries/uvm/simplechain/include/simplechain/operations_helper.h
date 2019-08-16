#pragma once
#include <simplechain/operations.h>
#include <simplechain/asset.h>

namespace simplechain {
	class operations_helper {
	private:
		operations_helper() {}
	public:
		static register_account_operation register_account(const std::string& account_address, const std::string& pubkey_hex);
		static mint_operation mint(const std::string& account_address, asset_id_t asset_id, share_type amount);
		static transfer_operation transfer(const std::string& from_address, const std::string& to_address, asset_id_t asset_id, share_type amount);
		static contract_create_operation create_contract_from_file(const std::string& caller_addr, const std::string& contract_filepath,
			gas_count_type gas_limit = 50000, gas_price_type gas_price = 10);
		static native_contract_create_operation create_native_contract(const std::string& caller_addr, const std::string& template_key,
			gas_count_type gas_limit = 50000, gas_price_type gas_price = 10);
		static contract_create_operation create_contract(const std::string& caller_addr, const uvm::blockchain::Code& contract_code,
			gas_count_type gas_limit = 50000, gas_price_type gas_price = 10);
		static contract_invoke_operation invoke_contract(const std::string& caller_addr, const std::string& contract_address,
			const std::string& contract_api_name, const fc::variants& api_args,
			gas_count_type gas_limit = 50000, gas_price_type gas_price = 10, asset_id_t deposit_asset_id = 0, share_type deposit_amount=0);
	};
}
