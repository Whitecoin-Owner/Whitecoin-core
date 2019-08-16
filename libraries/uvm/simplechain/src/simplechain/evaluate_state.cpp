#include <simplechain/evaluate_state.h>
#include <simplechain/transaction.h>
#include <simplechain/blockchain.h>
#include <iostream>

namespace simplechain {
	evaluate_state::evaluate_state(blockchain* chain_, transaction* tx_)
		: chain(chain_), current_tx(tx_) {
	}
	blockchain* evaluate_state::get_chain() const {
		return chain;
	}
	void evaluate_state::set_current_tx(transaction* tx) {
		this->current_tx = tx;
	}
	transaction* evaluate_state::get_current_tx() const {
		return current_tx;
	}
	StorageDataType evaluate_state::get_storage(const std::string& contract_address, const std::string& key) const {
		// TODO: read from parent evaluate_state first
		auto it1 = invoke_contract_result.storage_changes.find(contract_address);
		if (it1 != invoke_contract_result.storage_changes.end()) {
			auto& changes = it1->second;
			auto it2 = changes.find(key);
			if (it2 != changes.end()) {
				auto& change = *it2;
				return change.second;
			}
		}
		return chain->get_storage(contract_address, key);
	}

	std::map<std::string, StorageDataType> evaluate_state::get_contract_storages(const std::string& contract_addr) const {
		return chain->get_contract_storages(contract_addr);
	}

	void evaluate_state::emit_event(const std::string& contract_address, const std::string& event_name, const std::string& event_arg) {
		contract_event_notify_info event_notify_info;
		event_notify_info.block_num = get_chain()->latest_block().block_number + 1;
		event_notify_info.caller_addr = caller_address;
		event_notify_info.contract_address = contract_address;
		event_notify_info.event_name = event_name;
		event_notify_info.event_arg = event_arg;
		invoke_contract_result.events.push_back(event_notify_info);
	}

	void evaluate_state::store_contract(const std::string& contract_address, const contract_object& contract_obj) {
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.first == contract_address) {
				p.second = contract_obj;
				return;
			}
		}
		invoke_contract_result.new_contracts.push_back(std::make_pair(contract_address, contract_obj));
	}

	std::shared_ptr<contract_object> evaluate_state::get_contract_by_address(const std::string& contract_address) const {
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.first == contract_address) {
				return std::make_shared<contract_object>(p.second);
			}
		}
		return chain->get_contract_by_address(contract_address);
	}

	std::shared_ptr<contract_object> evaluate_state::get_contract_by_name(const std::string& name) const {
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.second.contract_name == name) {
				return std::make_shared<contract_object>(p.second);
			}
		}
		return chain->get_contract_by_name(name);
	}

	void evaluate_state::update_account_asset_balance(const std::string& account_address, asset_id_t asset_id, int64_t balance_change) {
		for (auto& p : invoke_contract_result.account_balances_changes) {
			if (p.first.first == account_address && p.first.second == asset_id) {
				p.second += balance_change;
				return;
			}
		}
		auto key = std::make_pair(account_address, asset_id);
		invoke_contract_result.account_balances_changes[key] = balance_change;
	}

	share_type evaluate_state::get_account_asset_balance(const std::string& account_address, asset_id_t asset_id) const {
		auto balance = amount_change_type(chain->get_account_asset_balance(account_address, asset_id));
		for (const auto& p : invoke_contract_result.account_balances_changes) {
			balance += p.second;
		}
		// TODO: use parent evaluate_state's cache to update balance query result
		return share_type(balance);
	}

	void evaluate_state::set_contract_storage_changes(const std::string& contract_address, const contract_storage_changes_type& changes) {
		invoke_contract_result.storage_changes[contract_address] = changes;
	}

}
