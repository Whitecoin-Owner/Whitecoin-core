#pragma once

#include <native_contract/native_contract_api.h>
#include <native_contract/native_token_contract.h>

namespace uvm {
	namespace contract {

		// this is native contract for uniswap   
		class uniswap_native_contract : public token_native_contract
		{
		
		public:
			static std::string native_contract_key() { return "uniswap"; }
			uniswap_native_contract(std::shared_ptr<native_contract_interface> proxy) : token_native_contract(proxy){}
			virtual ~uniswap_native_contract() {}

			virtual std::shared_ptr<native_contract_interface> get_proxy() const { return _proxy; }
			
			virtual std::string contract_key() const;
			virtual std::set<std::string> apis() const;
			virtual std::set<std::string> offline_apis() const;
			virtual std::set<std::string> events() const;

			virtual void invoke(const std::string& api_name, const std::string& api_arg);

			void init_api(const std::string& api_name, const std::string& api_arg);
			void init_config_api(const std::string& api_name, const std::string& api_arg);
			void setMinAddAmount_api(const std::string& api_name, const std::string& api_arg);
			void addLiquidity_api(const std::string& api_name, const std::string& api_arg);
			void removeLiquidity_api(const std::string& api_name, const std::string& api_arg);
			void withdraw_api(const std::string& api_name, const std::string& api_arg);
			void on_deposit_asset_api(const std::string& api_name, const std::string& api_args);
			
			void caculatePoolShareByToken_api(const std::string& api_name, const std::string& api_arg);
			void getUserRemoveableLiquidity_api(const std::string& api_name, const std::string& api_arg);
			void balanceOfAsset_api(const std::string& api_name, const std::string& api_arg);
			void caculateExchangeAmount_api(const std::string& api_name, const std::string& api_arg);
			void getInfo_api(const std::string& api_name, const std::string& api_arg);
			
		};

	}
}
