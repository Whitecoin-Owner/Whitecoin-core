#include <simplechain/rpcserver.h>
#include <simplechain/chain_rpc.h>
#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <fc/io/json.hpp>

namespace simplechain {
	using namespace std;
	using namespace rpc;

	static std::map<std::string, RpcHandlerType> rpc_methods = {

	{ "mint", &mint },
	{ "add_asset", &add_asset },
	{ "register_account", &register_account },
	{ "transfer", &transfer },
	{ "create_contract_from_file", &create_contract_from_file },
	{ "create_native_contract", &create_native_contract },
	{ "create_contract", &create_contract },
	{ "invoke_contract", &invoke_contract },
	{ "invoke_contract_offline", &invoke_contract_offline },
	{ "exit", &exit_chain },
	{ "generate_block", &generate_block },
	{ "get_block_by_height", &get_block_by_height },
	{ "get_tx", &get_tx },
	{ "get_tx_receipt", &get_tx_receipt },
	{ "get_chain_state", &get_chain_state },
	{ "list_accounts", &list_accounts },
	{ "list_assets", &list_assets },
	{ "list_contracts", &list_contracts },
	{ "get_contract_info", &get_contract_info },
	{ "get_account_balances", &get_account_balances },
	{ "get_contract_storages", &get_contract_storages },
	{ "get_storage", &get_storage },
	{ "add_asset", &add_asset },
        { "generate_key", &generate_key },
        { "sign_info", &sign_info },
	
        //add debug rpc
	{ "set_breakpoint", &set_breakpoint },
	{ "view_debug_info", &view_debug_info },
	{ "view_localvars_in_last_debugger_state", &view_localvars_in_last_debugger_state },
	{ "view_upvalues_in_last_debugger_state", &view_upvalues_in_last_debugger_state },
	{ "debugger_step_out", &debugger_step_out },
	{ "debugger_step_into", &debugger_step_into },
	{ "debugger_step_over", &debugger_step_over },
	{ "debugger_go_resume", &debugger_go_resume },
	{ "get_breakpoints_in_last_debugger_state", &get_breakpoints_in_last_debugger_state },
	{ "remove_breakpoint_in_last_debugger_state", &remove_breakpoint_in_last_debugger_state },
	{ "clear_breakpoints_in_last_debugger_state", &clear_breakpoints_in_last_debugger_state },
	{ "debugger_invoke_contract", &debugger_invoke_contract },
	{ "view_current_contract_storage_value", &view_current_contract_storage_value },
	{ "view_call_stack", &view_call_stack },

	{ "load_contract_state", &load_contract_state },
    { "load_new_contract_from_json", &load_new_contract_from_json }

	};

	RpcServer::RpcServer(blockchain* chain, int port)
		: _chain(chain), _port(port) {
		_server = std::make_shared<HttpServer>();
	}
	RpcServer::~RpcServer() {

	}

	static std::string read_all_string_from_stream(const std::istream& stream) {
		std::vector<char> data(std::istreambuf_iterator<char>(stream.rdbuf()),
			std::istreambuf_iterator<char>());
		std::string str(data.begin(), data.end());
		return str;
	}

	static fc::variant read_json_from_stream(const std::istream& stream) {
		return fc::json::from_string(read_all_string_from_stream(stream));
	}

	static void params_assert(bool cond, const std::string& msg = "") {
		if (!cond) {
			throw uvm::core::UvmException(msg.empty() ? "params invalid" : msg);
		}
	}

	static RpcRequest read_rpc_request_from_stream(const std::istream& stream) {
		auto json_val = read_json_from_stream(stream);
		params_assert(json_val.is_object());
		auto json_obj = json_val.as<fc::mutable_variant_object>();
		params_assert(json_obj.find("method") != json_obj.end() && json_obj["method"].is_string());
		auto method = json_obj["method"].as_string();
		fc::variants params;
		if (json_obj.find("params") != json_obj.end() && json_obj["params"].is_array()) {
			params = json_obj["params"].as<fc::variants>();
		}
		RpcRequest req;
		req.method = method;
		req.params = params;
		return req;
	}

	static void send_rpc_response(shared_ptr<HttpServer::Response> response, const RpcResponse& rpc_response) {
		fc::mutable_variant_object res_json;
		res_json["result"] = rpc_response.result;
		res_json["code"] = rpc_response.error_code;
		if (rpc_response.has_error) {
			res_json["message"] = rpc_response.error;
			res_json["error"] = rpc_response.error;
		}
		auto res_json_str = fc::json::to_string(res_json);
		*response << "HTTP/1.1 200 OK\r\n"
			<< "Content-Length: " << res_json_str.length() << "\r\n\r\n"
			<< res_json_str;
	}

	void RpcServer::start() {
		_server->config.address = "0.0.0.0";
		_server->config.port = _port;

		_server->resource["^/api"]["POST"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
			try {
				auto rpc_req = read_rpc_request_from_stream(request->content);

				auto res_str = std::string("method: ") + rpc_req.method;
				RpcResponse rpc_res;

				params_assert(rpc_methods.find(rpc_req.method) != rpc_methods.end());
				auto handler = rpc_methods[rpc_req.method];
				try {
					auto result = handler(this->_chain, this->_server.get(), rpc_req.params);
					rpc_res.result = result;
				}
				catch (const fc::exception& e) {
					rpc_res.has_error = true;
					rpc_res.error = e.to_detail_string();
					rpc_res.error_code = 100;
				}
				catch (const std::exception& e) {
					rpc_res.has_error = true;
					rpc_res.error = e.what();
					rpc_res.error_code = 100;
				}

				send_rpc_response(response, rpc_res);
			}
			catch (const exception &e) {
				*response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
					<< e.what();
			}
		};

		_server->default_resource["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
			std::string msg("please visit /api to use http rpc services");
			*response << "HTTP/1.1 200 OK\r\n"
				<< "Content-Length: " << msg.length() << "\r\n\r\n"
				<< msg;
		};

		_server->on_error = [](shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/) {
			// Handle errors here
			// Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
		};

		thread server_thread([&]() {
			_server->start();
		});

		// Wait for server to start so that the client can connect
		this_thread::sleep_for(chrono::seconds(1));
		server_thread.join();
	}
}
