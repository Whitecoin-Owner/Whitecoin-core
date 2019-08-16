#pragma once
#include <simplechain/operations_helper.h>
#include <simplechain/blockchain.h>
#include <simplechain/block.h>
#include <simplechain/transaction.h>
#include <string>
#include <vector>
#include <memory>
#include <boost/functional.hpp>
#include <boost/function.hpp>
#include <server_http.hpp>

namespace simplechain {

	using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

	namespace rpc {

		typedef std::vector<fc::variant> RpcRequestParams;

		struct RpcRequest {
			std::string method;
			RpcRequestParams params;
		};

		typedef fc::variant RpcResultType;

		struct RpcResponse {
			bool has_error = false;
			std::string error;
			int32_t error_code = 0;
			RpcResultType result;
		};

		typedef boost::function<RpcResultType(blockchain*, HttpServer*, const RpcRequestParams&)> RpcHandlerType;
		// get_account(address: string)
		RpcResultType get_account(blockchain* chain, HttpServer* server, const RpcRequestParams& param);
		// register_account(address: string, pub_key_hex: string)
		RpcResultType register_account(blockchain* chain, HttpServer* server, const RpcRequestParams& param);
		// mint(caller_address: string, asset_id: int, amount: int)
		RpcResultType mint(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// add_asset(asset_symbol: string, precision: int)
		RpcResultType add_asset(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// transfer(from_address: string, to_address: string, asset_id: int, amount: int)
		RpcResultType transfer(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// create_contract_from_file(caller_address: string, contract_filepath: string, gas_limit: int, gas_price: int)
		RpcResultType create_contract_from_file(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		
		RpcResultType create_native_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// create_contract(caller_address: string, contract_base64: string, gas_limit: int, gas_price: int)
		RpcResultType create_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// invoke_contract(caller_address: string, contract_address: string, api_name: string, api_args: [string], deposit_asset_id: int, deposit_amount: int, gas_limit: int, gas_price: int)
		RpcResultType invoke_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType invoke_contract_offline(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType generate_block(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_block_by_height(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_tx(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_tx_receipt(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType exit_chain(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_chain_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType list_accounts(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType list_assets(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType list_contracts(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_contract_info(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_account_balances(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_contract_storages(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_storage(blockchain* chain, HttpServer* server, const RpcRequestParams& params);

		//add debug rpc
		RpcResultType set_breakpoint(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_breakpoints_in_last_debugger_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType view_localvars_in_last_debugger_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType view_debug_info(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType debugger_step_out(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType debugger_step_over(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType debugger_step_into(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType debugger_go_resume(blockchain* chain, HttpServer* server, const RpcRequestParams& params);

		RpcResultType remove_breakpoint_in_last_debugger_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType clear_breakpoints_in_last_debugger_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType debugger_invoke_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType view_upvalues_in_last_debugger_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType view_current_contract_storage_value(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType view_call_stack(blockchain* chain, HttpServer* server, const RpcRequestParams& params);

		RpcResultType generate_key(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType sign_info(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// TODO: deposit to contract


		// load_contract_state(contract_address: string, contract_state_json_string: string)
		RpcResultType load_contract_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType load_new_contract_from_json(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		
	}
}
