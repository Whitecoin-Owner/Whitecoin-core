#include <simplechain/contract.h>
#include <simplechain/contract_entry.h>
#include <simplechain/blockchain.h>
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>
#include <uvm/uvm_lutil.h>
#include <uvm/uvm_lib.h>

namespace simplechain {
	contract_create_operation::contract_create_operation()
	: type(operation_type_enum::CONTRACT_CREATE),
	  gas_price(0),
	  gas_limit(0) {}

	contract_create_operation::~contract_create_operation() {

	}
	std::string contract_create_operation::calculate_contract_id() const
	{
		std::string id;
		fc::sha256::encoder enc;
		FC_ASSERT((contract_code != uvm::blockchain::Code()));
		if (contract_code != uvm::blockchain::Code())
		{
			std::pair<uvm::blockchain::Code, fc::time_point_sec> info_to_digest(contract_code, op_time);
			fc::raw::pack(enc, info_to_digest);
		}

		id = fc::ripemd160::hash(enc.result()).str();
		return std::string(SIMPLECHAIN_CONTRACT_ADDRESS_PREFIX) + id;
	}

	native_contract_create_operation::native_contract_create_operation()
		: type(operation_type_enum::CONTRACT_CREATE),
		gas_price(0),
		gas_limit(0) {}

	native_contract_create_operation::~native_contract_create_operation() {

	}
	std::string native_contract_create_operation::calculate_contract_id() const
	{
		std::string id;
		fc::sha256::encoder enc;
		FC_ASSERT(!template_key.empty());

		std::pair<std::string, fc::time_point_sec> info_to_digest(template_key, op_time);
		fc::raw::pack(enc, info_to_digest);

		id = fc::ripemd160::hash(enc.result()).str();
		return std::string(SIMPLECHAIN_CONTRACT_ADDRESS_PREFIX) + id;
	}

	contract_invoke_operation::contract_invoke_operation()
	: type(operation_type_enum::CONTRACT_INVOKE),
	  gas_price(0),
	  gas_limit(0) {}

	contract_invoke_operation::~contract_invoke_operation() {

	}


	void contract_invoke_result::reset()
	{
		api_result.clear();
		storage_changes.clear();
		account_balances_changes.clear();
		transfer_fees.clear();
		events.clear();
		new_contracts.clear();
		exec_succeed = true;
	}

	void contract_invoke_result::set_failed()
	{
		api_result.clear();
		storage_changes.clear();
		account_balances_changes.clear();
		transfer_fees.clear();
		events.clear();
		new_contracts.clear();
		exec_succeed = false;
	}

	void contract_invoke_result::validate() {
		// in >= out + fee
		std::map<asset_id_t, amount_change_type> in_totals;
		std::map<asset_id_t, amount_change_type> out_totals;
		for (const auto& p : account_balances_changes) {
			auto asset = p.first.second;
			auto change = p.second;
			if (change < 0) {
				// in
				amount_change_type total = 0;
				if (in_totals.find(asset) != in_totals.end())
					total = in_totals[asset];
				total += (-change);
				in_totals[asset] = total;
			}
			else if (change > 0) {
				// out
				amount_change_type total = 0;
				if (out_totals.find(asset) != out_totals.end())
					total = out_totals[asset];
				total += change;
				out_totals[asset] = total;
			}
		}
		// add fees to out
		for (const auto& p : transfer_fees) {
			auto asset = p.first;
			auto change = p.second;
			amount_change_type total = 0;
			if (out_totals.find(asset) != out_totals.end())
				total = out_totals[asset];
			total += amount_change_type(change);
		}
		// each asset in out must have large in
		for (const auto& p : out_totals) {
			auto asset = p.first;
			auto out_total = p.second;
			if (out_total == 0) {
				continue;
			}
			if (in_totals.find(asset) == in_totals.end() || in_totals[asset] < out_total) {
				throw uvm::core::UvmException("contract evaluate result must in >= out + fee");
			}
		}
	}

	void contract_invoke_result::apply_pendings(blockchain* chain, const std::string& tx_id) {
		auto tx_receipt = chain->get_tx_receipt(tx_id);
		if (!tx_receipt) {
			tx_receipt = std::make_shared<transaction_receipt>();
			tx_receipt->tx_id = tx_id;
		}
		for (const auto& item : new_contracts) {
			chain->store_contract(item.first, item.second);
		}
		for (const auto& p : events) {
			tx_receipt->events.push_back(p);
		}
		tx_receipt->exec_succeed = this->exec_succeed;
		for (const auto& p : account_balances_changes) {
			auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount_change = p.second;
			chain->update_account_asset_balance(addr, asset_id, amount_change);
		}
		for (const auto& p : storage_changes) {
			const auto& contract_address = p.first;
			const auto& changes = p.second;
			for (const auto& it : changes) {
				chain->set_storage(contract_address, it.first, it.second.after);
			}
		}
		chain->set_tx_receipt(tx_id, *tx_receipt);
	}

	int64_t contract_invoke_result::count_storage_gas() const {
		cbor_diff::CborDiff differ;
		int64_t storage_gas = 0;
		uvm::lua::lib::UvmStateScope scope;
		for (auto all_con_chg_iter = storage_changes.begin(); all_con_chg_iter != storage_changes.end(); ++all_con_chg_iter)
		{
			// commit change to evaluator
			contract_storage_changes_type contract_storage_change;
			std::string contract_id = all_con_chg_iter->first;
			auto contract_change = all_con_chg_iter->second;
			cbor::CborMapValue nested_changes;
			jsondiff::JsonObject json_nested_changes;

			for (auto con_chg_iter = contract_change.begin(); con_chg_iter != contract_change.end(); ++con_chg_iter)
			{
				std::string contract_name = con_chg_iter->first;

				StorageDataChangeType storage_change;
				// storage_op from before and after to diff
				auto storage_after = con_chg_iter->second.after;
				auto cbor_storage_before = uvm_storage_value_to_cbor(StorageDataType::create_lua_storage_from_storage_data(scope.L(), con_chg_iter->second.before));
				auto cbor_storage_after = uvm_storage_value_to_cbor(StorageDataType::create_lua_storage_from_storage_data(scope.L(), con_chg_iter->second.after));
				con_chg_iter->second.cbor_diff = *(differ.diff(cbor_storage_before, cbor_storage_after));
				auto cbor_diff_value = std::make_shared<cbor::CborObject>(con_chg_iter->second.cbor_diff.value());
				const auto& cbor_diff_chars = cbor_diff::cbor_encode(cbor_diff_value);
				storage_change.storage_diff.storage_data = cbor_diff_chars;
				storage_change.after = storage_after;
				contract_storage_change[contract_name] = storage_change;
				nested_changes[contract_name] = cbor_diff_value;

			}
			// count gas by changes size
			size_t changes_size = 0;
			auto nested_changes_cbor = cbor::CborObject::create_map(nested_changes);
			const auto& changes_parsed_to_array = uvm::util::nested_cbor_object_to_array(nested_changes_cbor.get());
			changes_size = cbor_diff::cbor_encode(changes_parsed_to_array).size();
			// printf("changes size: %d bytes\n", changes_size);
			storage_gas += changes_size * 10; // 1 byte storage cost 10 gas

			if (storage_gas < 0) {
				return -1;
			}
		}
		dlog("storage gas: " + std::to_string(storage_gas));
		return storage_gas;
	}

	int64_t contract_invoke_result::count_event_gas() const {
		int64_t total_event_gas = 0;
		for (const auto& event_change : events) {
			auto event_size = event_change.event_name.size() + event_change.event_arg.size();
			auto event_gas = event_size * 2; // 1 byte event cost 2 gas
			total_event_gas += event_gas;
			if (total_event_gas < 0) {
				return -1;
			}
		}
		dlog("event gas: " + std::to_string(total_event_gas));
		return total_event_gas;
	}

	fc::mutable_variant_object contract_event_notify_info::to_json() const {
		fc::mutable_variant_object info;
		info["contract_address"] = contract_address;
		info["event_name"] = event_name;
		info["event_arg"] = event_arg;
		info["caller_addr"] = caller_addr;
		info["block_num"] = block_num;
		return info;
	}

	fc::mutable_variant_object transaction_receipt::to_json() const {
		fc::mutable_variant_object info;
		info["tx_id"] = tx_id;
		fc::variants events_json;
		for (const auto& event_data : events) {
			events_json.push_back(event_data.to_json());
		}
		info["events"] = events_json;
		info["exec_succeed"] = exec_succeed;
		return info;
	}

}
