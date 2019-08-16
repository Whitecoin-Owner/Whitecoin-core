#pragma once
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <cborcpp/cbor.h>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <uvm/uvm_api.h>
#include <simplechain/contract.h>
#include <simplechain/storage.h>
#include <simplechain/evaluate_state.h>
#include <native_contract/native_contract_api.h>

namespace simplechain {

	class native_contract_store : public uvm::contract::native_contract_interface
	{
	protected:
		evaluate_state* _evaluate;
		contract_invoke_result _contract_invoke_result;
	public:
		std::string contract_id;

		native_contract_store() : _evaluate(nullptr), contract_id("") {}
		native_contract_store(evaluate_state* evaluate, const std::string& _contract_id) : _evaluate(evaluate), contract_id(_contract_id) {}
		virtual ~native_contract_store() {}

		void init_config(evaluate_state* evaluate, const std::string& _contract_id);

		virtual std::shared_ptr<uvm::contract::native_contract_interface> get_proxy() const {
			return nullptr;
		}

		virtual std::string contract_key() const {
			return "abstract";
		}

		virtual std::set<std::string> apis() const {
			return std::set<std::string>{};
		}
		virtual std::set<std::string> offline_apis() const {
			return std::set<std::string>{};
		}
		virtual std::set<std::string> events() const {
			return std::set<std::string>{};
		}

		virtual void invoke(const std::string& api_name, const std::string& api_arg) {
			throw_error("can't call native_contract_store::invoke");
		}

		virtual address contract_address() const;
		virtual uint64_t gas_count_for_api_invoke(const std::string& api_name) const
		{
			return 100; // now all native api call requires 100 gas count
		}

		virtual void set_contract_storage(const address& contract_address, const std::string& storage_name, const StorageDataType& value);
		virtual void set_contract_storage(const address& contract_address, const std::string& storage_name, cbor::CborObjectP cbor_value);
		virtual void transfer_to_address(const address& from_contract_address, const address& to_address, const std::string& asset_symbol, const uint64_t amount);
		virtual void fast_map_set(const address& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value);
		virtual StorageDataType get_contract_storage(const address& contract_address, const std::string& storage_name) const;
		virtual cbor::CborObjectP get_contract_storage_cbor(const address& contract_address, const std::string& storage_name) const;
		virtual cbor::CborObjectP fast_map_get(const address& contract_address, const std::string& storage_name, const std::string& key) const;
		virtual std::string get_string_contract_storage(const address& contract_address, const std::string& storage_name) const;
		virtual int64_t get_int_contract_storage(const address& contract_address, const std::string& storage_name) const;

		virtual void emit_event(const address& contract_address, const std::string& event_name, const std::string& event_arg);

		virtual void current_fast_map_set(const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
			fast_map_set(contract_address(), storage_name, key, cbor_value);
		}
		virtual StorageDataType get_current_contract_storage(const std::string& storage_name) const {
			return get_contract_storage(contract_address(), storage_name);
		}
		virtual cbor::CborObjectP get_current_contract_storage_cbor(const std::string& storage_name) const {
			return get_contract_storage_cbor(contract_address(), storage_name);
		}
		virtual cbor::CborObjectP current_fast_map_get(const std::string& storage_name, const std::string& key) const {
			return fast_map_get(contract_address(), storage_name, key);
		}
		virtual std::string get_string_current_contract_storage(const std::string& storage_name) const {
			return get_string_contract_storage(contract_address(), storage_name);
		}
		virtual int64_t get_int_current_contract_storage(const std::string& storage_name) const {
			return get_int_contract_storage(contract_address(), storage_name);
		}
		virtual void set_current_contract_storage(const std::string& storage_name, cbor::CborObjectP cbor_value) {
			set_contract_storage(contract_address(), storage_name, cbor_value);
		}
		virtual void set_current_contract_storage(const std::string& storage_name, const StorageDataType& value) {
			set_contract_storage(contract_address(), storage_name, value);
		}
		virtual void current_transfer_to_address(const std::string& to_address, const std::string& asset_symbol, uint64_t amount) {
			transfer_to_address(contract_address(), to_address, asset_symbol, amount);
		}
		virtual void current_set_on_deposit_asset(const std::string& asset_symbol, uint64_t amount) {
			transfer_to_address(caller_address(), contract_address(), asset_symbol, amount);
		}
		virtual void emit_event(const std::string& event_name, const std::string& event_arg) {
			emit_event(contract_address(), event_name, event_arg);
		}

		virtual uint64_t head_block_num() const;
		virtual std::string caller_address_string() const;
		virtual address caller_address() const;
		virtual void throw_error(const std::string& err) const;

		virtual void add_gas(uint64_t gas);
		virtual void set_invoke_result_caller();

		virtual void* get_result() { return &_contract_invoke_result; }
		virtual void set_api_result(const std::string& api_result) {
			_contract_invoke_result.api_result = api_result;
		}

		virtual bool is_valid_address(const std::string& addr);
		virtual uint32_t get_chain_now() const;
	};

	class native_contract_finder
	{
	public:
		static bool has_native_contract_with_key(const std::string& key);
		static std::shared_ptr<uvm::contract::native_contract_interface> create_native_contract_by_key(evaluate_state* evaluate, const std::string& key, const address& contract_address);
	};


}
