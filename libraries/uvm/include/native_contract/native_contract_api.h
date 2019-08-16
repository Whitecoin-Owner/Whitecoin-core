#pragma once
#include <string>
#include <set>
#include <cstdint>
#include <memory>
#include <cborcpp/cbor.h>

namespace uvm {
	namespace contract {
		class native_contract_interface
		{
		public:
			// unique key to identify native contract
			virtual std::string contract_key() const = 0;

			virtual std::set<std::string> apis() const = 0;
			virtual std::set<std::string> offline_apis() const = 0;
			virtual std::set<std::string> events() const = 0;

			// @throw std::exception
			virtual void invoke(const std::string& api_name, const std::string& api_arg) = 0;

			virtual uint64_t gas_count_for_api_invoke(const std::string& api_name) const = 0;

			bool has_api(const std::string& api_name) const {
				const auto& api_names = apis();
				return api_names.find(api_name) != api_names.end();
			}

			virtual std::shared_ptr<native_contract_interface> get_proxy() const = 0;

			virtual void current_fast_map_set(const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) = 0;
			virtual cbor::CborObjectP get_current_contract_storage_cbor(const std::string& storage_name) const = 0;
			virtual cbor::CborObjectP current_fast_map_get(const std::string& storage_name, const std::string& key) const = 0;
			virtual std::string get_string_current_contract_storage(const std::string& storage_name) const = 0;
			virtual int64_t get_int_current_contract_storage(const std::string& storage_name) const = 0;
			virtual void set_current_contract_storage(const std::string& storage_name, cbor::CborObjectP cbor_value) = 0;
			virtual void current_transfer_to_address(const std::string& to_address, const std::string& asset_symbol, uint64_t amount) = 0;
			virtual void current_set_on_deposit_asset(const std::string& asset_symbol, uint64_t amount) = 0;
			virtual void emit_event(const std::string& event_name, const std::string& event_arg) = 0;
			virtual uint64_t head_block_num() const = 0;
			virtual std::string caller_address_string() const = 0;
			virtual void throw_error(const std::string& err) const = 0;

			virtual void add_gas(uint64_t gas) = 0;
			virtual void set_invoke_result_caller() = 0;
			// @return contract_invoke_result*
			virtual void* get_result() = 0;
			virtual void set_api_result(const std::string& api_result) = 0;
			virtual bool is_valid_address(const std::string& addr) = 0;

			virtual uint32_t get_chain_now() const = 0;
		};

		class abstract_native_contract_impl : public native_contract_interface {
		public:
			virtual uint64_t gas_count_for_api_invoke(const std::string& api_name) const {
				return get_proxy()->gas_count_for_api_invoke(api_name);
			}
			virtual void current_fast_map_set(const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
				get_proxy()->current_fast_map_set(storage_name, key, cbor_value);
			}
			virtual cbor::CborObjectP get_current_contract_storage_cbor(const std::string& storage_name) const {
				return get_proxy()->get_current_contract_storage_cbor(storage_name);
			}
			virtual cbor::CborObjectP current_fast_map_get(const std::string& storage_name, const std::string& key) const {
				return get_proxy()->current_fast_map_get(storage_name, key);
			}
			virtual std::string get_string_current_contract_storage(const std::string& storage_name) const {
				return get_proxy()->get_string_current_contract_storage(storage_name);
			}
			virtual int64_t get_int_current_contract_storage(const std::string& storage_name) const {
				return get_proxy()->get_int_current_contract_storage(storage_name);
			}
			virtual void set_current_contract_storage(const std::string& storage_name, cbor::CborObjectP cbor_value) {
				get_proxy()->set_current_contract_storage(storage_name, cbor_value);
			}
			virtual void current_transfer_to_address(const std::string& to_address, const std::string& asset_symbol, uint64_t amount) {
				get_proxy()->current_transfer_to_address(to_address, asset_symbol, amount);
			}
			virtual void current_set_on_deposit_asset(const std::string& asset_symbol, uint64_t amount) {
				get_proxy()->current_set_on_deposit_asset(asset_symbol, amount);
			}
			virtual void emit_event(const std::string& event_name, const std::string& event_arg) {
				get_proxy()->emit_event(event_name, event_arg);
			}

			virtual uint64_t head_block_num() const {
				return get_proxy()->head_block_num();
			}
			virtual std::string caller_address_string() const {
				return get_proxy()->caller_address_string();
			}
			virtual void throw_error(const std::string& err) const {
				get_proxy()->throw_error(err);
			}

			virtual void add_gas(uint64_t gas) {
				get_proxy()->add_gas(gas);
			}
			virtual void set_invoke_result_caller() {
				get_proxy()->set_invoke_result_caller();
			}
			virtual void* get_result() {
				return get_proxy()->get_result();
			}
			virtual void set_api_result(const std::string& api_result) {
				get_proxy()->set_api_result(api_result);
			}
			virtual bool is_valid_address(const std::string& addr) {
				return get_proxy()->is_valid_address(addr);
			}

			virtual uint32_t get_chain_now() const {
				return get_proxy()->get_chain_now();
			}
		};
	}
}
