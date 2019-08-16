#include <simplechain/operations_helper.h>
#include <simplechain/contract_helper.h>

namespace simplechain {
	register_account_operation operations_helper::register_account(const std::string& account_address, const std::string& pubkey_hex) {
		register_account_operation op;
		op.account_address = account_address;
		op.pubkey_hex = pubkey_hex;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		return op;
	}

	mint_operation operations_helper::mint(const std::string& account_address, asset_id_t asset_id, share_type amount) {
		mint_operation op;
		op.account_address = account_address;
		op.amount = amount;
		op.asset_id = asset_id;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		return op;
	}

	transfer_operation operations_helper::transfer(const std::string& from_address, const std::string& to_address, asset_id_t asset_id, share_type amount) {
		transfer_operation op;
		op.from_address = from_address;
		op.to_address = to_address;;
		op.amount = amount;
		op.asset_id = asset_id;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		return op;
	}

	contract_create_operation operations_helper::create_contract_from_file(const std::string& caller_addr, const std::string& contract_filepath,
		gas_count_type gas_limit, gas_price_type gas_price) {
		contract_create_operation op;
		op.caller_address = caller_addr;
		auto contract = ContractHelper::load_contract_from_file(contract_filepath);
		op.contract_code = contract;
		op.gas_limit = gas_limit;
		op.gas_price = gas_price;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		return op;
	}

	native_contract_create_operation operations_helper::create_native_contract(const std::string& caller_addr, const std::string& template_key,
		gas_count_type gas_limit, gas_price_type gas_price) {
		native_contract_create_operation op;
		op.caller_address = caller_addr;
		op.template_key = template_key;
		op.gas_limit = gas_limit;
		op.gas_price = gas_price;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		return op;
	}

	contract_create_operation operations_helper::create_contract(const std::string& caller_addr, const uvm::blockchain::Code& contract_code,
		gas_count_type gas_limit, gas_price_type gas_price) {
		contract_create_operation op;
		op.caller_address = caller_addr;
		op.contract_code = contract_code;
		op.gas_limit = gas_limit;
		op.gas_price = gas_price;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		return op;
	}

	contract_invoke_operation operations_helper::invoke_contract(const std::string& caller_addr, const std::string& contract_address,
		const std::string& contract_api_name, const fc::variants& api_args,
		gas_count_type gas_limit, gas_price_type gas_price, asset_id_t deposit_asset_id, share_type deposit_amount) {
		contract_invoke_operation op;
		op.caller_address = caller_addr;
		op.contract_address = contract_address;
		op.contract_api = contract_api_name;
		op.contract_args = api_args;
		op.gas_limit = gas_limit;
		op.gas_price = gas_price;
		op.deposit_asset_id = deposit_asset_id;
		op.deposit_amount;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		return op;
	}
}
