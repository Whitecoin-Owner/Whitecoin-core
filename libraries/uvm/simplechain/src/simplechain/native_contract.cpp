#include <simplechain/native_contract.h>
#include <native_contract/native_token_contract.h>
#include <native_contract/native_exchange_contract.h>
#include <native_contract/native_uniswap_contract.h>
#include <simplechain/blockchain.h>
#include <boost/algorithm/string.hpp>
#include <jsondiff/jsondiff.h>
#include <memory>

namespace simplechain {
	using namespace cbor_diff;
	using namespace cbor;
	using namespace std;
	using namespace uvm::contract;

	void native_contract_store::init_config(evaluate_state* evaluate, const std::string& _contract_id) {
		this->_evaluate = evaluate;
		this->contract_id = _contract_id;
	}

	void native_contract_store::set_contract_storage(const address& contract_address, const std::string& storage_name, const StorageDataType& value)
	{
		if (_contract_invoke_result.storage_changes.find(contract_address) == _contract_invoke_result.storage_changes.end())
		{
			_contract_invoke_result.storage_changes[contract_address] = contract_storage_changes_type();
		}
		auto& storage_changes = _contract_invoke_result.storage_changes[contract_address];
		if (storage_changes.find(storage_name) == storage_changes.end())
		{
			StorageDataChangeType change;
			change.after = value;
			const auto &before = _evaluate->get_storage(contract_address, storage_name);
			cbor_diff::CborDiff differ;
			const auto& before_cbor = cbor_decode(before.storage_data);
			const auto& after_cbor = cbor_decode(change.after.storage_data);
			auto diff = differ.diff(before_cbor, after_cbor);
			change.storage_diff.storage_data = cbor_encode(diff->value());
			change.before = before;
			storage_changes[storage_name] = change;
		}
		else
		{
			auto& change = storage_changes[storage_name];
			auto before = change.before;
			auto after = value;
			change.after = after;
			cbor_diff::CborDiff differ;
			const auto& before_cbor = cbor_diff::cbor_decode(before.storage_data);
			const auto& after_cbor = cbor_diff::cbor_decode(after.storage_data);
			auto diff = differ.diff(before_cbor, after_cbor);
			change.storage_diff.storage_data = cbor_encode(diff->value());
		}
	}

	void native_contract_store::set_contract_storage(const address& contract_address, const std::string& storage_name, cbor::CborObjectP cbor_value) {
		StorageDataType value;
		value.storage_data = cbor_encode(cbor_value);
		return set_contract_storage(contract_address, storage_name, value);
	}

	void native_contract_store::transfer_to_address(const address& from_contract_address, const address& to_address, const std::string& asset_symbol, const uint64_t amount) {
		auto a = _evaluate->get_chain()->get_asset_by_symbol(asset_symbol);
		if (a==nullptr) {
			throw_error(std::string("invalid asset_symbol ") + asset_symbol);
		}
		auto asset_id = a->asset_id;
		bool contract_balance_pair_found = false;
		bool to_pair_found = false;
		for (auto& p : _contract_invoke_result.account_balances_changes) {
			if (p.first.first == from_contract_address && p.first.second == asset_id) {
				p.second -= int64_t(amount);
				contract_balance_pair_found = true;
				continue;
			}
			if (p.first.first == to_address && p.first.second == asset_id) {
				p.second += amount;
				to_pair_found = true;
				continue;
			}
		}
		if (!contract_balance_pair_found) {
			auto p = std::make_pair(from_contract_address, asset_id);
			_contract_invoke_result.account_balances_changes[p] = - int64_t(amount);
		}
		if (!to_pair_found) {
			auto p = std::make_pair(to_address, asset_id);
			_contract_invoke_result.account_balances_changes[p] = int64_t(amount);
		}
	}

	void native_contract_store::fast_map_set(const address& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
		std::string full_key = storage_name + "." + key;
		set_contract_storage(contract_address, full_key, cbor_value);
	}

	StorageDataType native_contract_store::get_contract_storage(const address& contract_address, const std::string& storage_name) const
	{
		if (_contract_invoke_result.storage_changes.find(contract_address) == _contract_invoke_result.storage_changes.end())
		{
			return _evaluate->get_storage(contract_address, storage_name);
		}
		auto& storage_changes = _contract_invoke_result.storage_changes.at(contract_address);
		if (storage_changes.find(storage_name) == storage_changes.end())
		{
			return _evaluate->get_storage(contract_address, storage_name);
		}
		return storage_changes.at(storage_name).after;
	}

	cbor::CborObjectP native_contract_store::get_contract_storage_cbor(const address& contract_address, const std::string& storage_name) const {
		const auto& data = get_contract_storage(contract_address, storage_name);
		return cbor_decode(data.storage_data);
	}

	std::string native_contract_store::get_string_contract_storage(const address& contract_address, const std::string& storage_name) const {
		auto cbor_data = get_contract_storage_cbor(contract_address, storage_name);
		if (!cbor_data->is_string()) {
			throw_error(std::string("invalid string contract storage ") + contract_address + "." + storage_name);
		}
		return cbor_data->as_string();
	}

	cbor::CborObjectP native_contract_store::fast_map_get(const address& contract_address, const std::string& storage_name, const std::string& key) const {
		std::string full_key = storage_name + "." + key;
		return get_contract_storage_cbor(contract_address, full_key);
	}

	int64_t native_contract_store::get_int_contract_storage(const address& contract_address, const std::string& storage_name) const {
		auto cbor_data = get_contract_storage_cbor(contract_address, storage_name);
		if (!cbor_data->is_integer()) {
			throw_error(std::string("invalid int contract storage ") + contract_address + "." + storage_name);
		}
		return cbor_data->force_as_int();
	}

	void native_contract_store::emit_event(const address& contract_address, const std::string& event_name, const std::string& event_arg)
	{
		FC_ASSERT(!event_name.empty());
		contract_event_notify_info info;
		info.event_name = event_name;
		info.event_arg = event_arg;
		info.contract_address = contract_address;
		//info.caller_addr = caller_address->address_to_string();
		info.block_num = 1 + head_block_num();

		_contract_invoke_result.events.push_back(info);
	}

	uint64_t native_contract_store::head_block_num() const {
		return _evaluate->get_chain()->head_block_number();
	}

	std::string native_contract_store::caller_address_string() const {
		return _evaluate->caller_address;
	}

	address native_contract_store::caller_address() const {
		return _evaluate->caller_address;
	}

	address native_contract_store::contract_address() const {
		return contract_id;
	}

	void native_contract_store::throw_error(const std::string& err) const {
		printf("native contract error %s\n", err.c_str());
		throw uvm::core::UvmException(err);
		// FC_THROW_EXCEPTION(fc::assert_exception, err);
	}

	void native_contract_store::add_gas(uint64_t gas) {
		_contract_invoke_result.gas_used += gas;
	}

	void native_contract_store::set_invoke_result_caller() {
		_contract_invoke_result.invoker = caller_address();
	}

	bool native_contract_store::is_valid_address(const std::string& addr) {
		if (addr.compare(0, 2, "SL") == 0 || addr.compare(0, 3, "CON") == 0) {
			return true;
		}
		return false;
	}

	uint32_t native_contract_store::get_chain_now() const {
		return _evaluate->get_chain()->latest_block().block_time.sec_since_epoch();
	}

	// class native_contract_finder

	bool native_contract_finder::has_native_contract_with_key(const std::string& key)
	{
		std::vector<std::string> native_contract_keys = {
			// demo_native_contract::native_contract_key(),
			token_native_contract::native_contract_key(),
			exchange_native_contract::native_contract_key()
		};
		return std::find(native_contract_keys.begin(), native_contract_keys.end(), key) != native_contract_keys.end();
	}
	shared_ptr<native_contract_interface> native_contract_finder::create_native_contract_by_key(evaluate_state* evaluate, const std::string& key, const address& contract_address)
	{
		auto store = std::make_shared<native_contract_store>();
		store->init_config(evaluate, contract_address);

		shared_ptr<native_contract_interface> result;

		/*if (key == demo_native_contract::native_contract_key())
		{
			result = std::make_shared<demo_native_contract>(store);
		}
		else */
		if (key == token_native_contract::native_contract_key())
		{
			result = std::make_shared<token_native_contract>(store);
		}
		else if (key == exchange_native_contract::native_contract_key())
		{
			result = std::make_shared<exchange_native_contract>(store);
		}
		else if (key == uniswap_native_contract::native_contract_key())
		{
			result = std::make_shared<uniswap_native_contract>(store);
		}
		else
		{
			return nullptr;
		}
		return result;
	}
	

}
