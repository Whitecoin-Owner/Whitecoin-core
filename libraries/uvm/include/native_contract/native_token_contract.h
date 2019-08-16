#pragma once

#include <native_contract/native_contract_api.h>

namespace uvm {
	namespace contract {
		// this is native contract for token
		class token_native_contract : public uvm::contract::abstract_native_contract_impl
		{
		protected:
			std::shared_ptr<uvm::contract::native_contract_interface> _proxy;

		public:
			static std::string native_contract_key() { return "token"; }

			token_native_contract(std::shared_ptr<uvm::contract::native_contract_interface> proxy) : _proxy(proxy) {}
			virtual ~token_native_contract() {}

			virtual std::shared_ptr<uvm::contract::native_contract_interface> get_proxy() const { return _proxy; }

			virtual std::string contract_key() const;
			virtual std::set<std::string> apis() const;
			virtual std::set<std::string> offline_apis() const;
			virtual std::set<std::string> events() const;

			virtual void invoke(const std::string& api_name, const std::string& api_arg);
			std::string check_admin();
			std::string get_storage_state();
			std::string get_storage_token_name();
			std::string get_storage_token_symbol();
			int64_t get_storage_supply();
			int64_t get_storage_precision();
			int64_t get_balance_of_user(const std::string& owner_addr) const;
			cbor::CborMapValue get_allowed_of_user(const std::string& from_addr) const;
			std::string get_from_address();

			void init_api(const std::string& api_name, const std::string& api_arg);
			void init_token_api(const std::string& api_name, const std::string& api_arg);
			void transfer_api(const std::string& api_name, const std::string& api_arg);
			void balance_of_api(const std::string& api_name, const std::string& api_arg);
			void state_api(const std::string& api_name, const std::string& api_arg);
			void token_name_api(const std::string& api_name, const std::string& api_arg);
			void token_symbol_api(const std::string& api_name, const std::string& api_arg);
			void supply_api(const std::string& api_name, const std::string& api_arg);
			void totalSupply_api(const std::string& api_name, const std::string& api_arg);
			void precision_api(const std::string& api_name, const std::string& api_arg);
			// 授权另一个用户可以从自己的余额中提现
			// arg format : spenderAddress, amount(with precision)
			void approve_api(const std::string& api_name, const std::string& api_arg);
			// spender用户从授权人授权的金额中发起转账
			// arg format : fromAddress, toAddress, amount(with precision)
			void transfer_from_api(const std::string& api_name, const std::string& api_arg);
			// 查询一个用户被另外某个用户授权的金额
			// arg format : spenderAddress, authorizerAddress
			void approved_balance_from_api(const std::string& api_name, const std::string& api_arg);
			// 查询用户授权给其他人的所有金额
			// arg format : fromAddress
			void all_approved_from_user_api(const std::string& api_name, const std::string& api_arg);

		};
	}
}
