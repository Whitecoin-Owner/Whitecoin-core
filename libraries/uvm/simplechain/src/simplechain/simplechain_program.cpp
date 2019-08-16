#include <simplechain/simplechain.h>
#include <iostream>
#include <simplechain/rpcserver.h>
#include <uvm/uvm_lutil.h>
#include <fc/crypto/hex.hpp>
#include <cbor_diff/cbor_diff.h>
#include <cbor_diff/cbor_diff_tests.h>
#include <simplechain/native_contract_tests.h>

using namespace simplechain;
#ifndef RUN_BOOST_TESTS

int main(int argc, char** argv) {
	std::cout << "Hello, simplechain based on uvm" << std::endl;
	// cbor_diff::test_cbor_diff();
	// cbor_diff::test_cbor_json();
	// test_token_native_contract();
	try {
		auto chain = std::make_shared<simplechain::blockchain>();

		if (argc == 2) {
			// TODO: remove this demo code
			std::string contract1_addr;
			std::string caller_addr = std::string(SIMPLECHAIN_ADDRESS_PREFIX) + "caller1";

			{
				auto tx = std::make_shared<transaction>();
				auto op = operations_helper::mint(caller_addr, 0, 123);
				tx->tx_time = fc::time_point_sec(fc::time_point::now());
				tx->operations.push_back(op);

				chain->evaluate_transaction(tx);
				chain->accept_transaction_to_mempool(*tx);
			}
			{
				auto tx1 = std::make_shared<transaction>();
				std::string arg1(argv[1]);

				std::string contract1_gpc_filepath("../test/test_contracts/token2.lua.gpc");
				auto op = operations_helper::create_contract_from_file(caller_addr, contract1_gpc_filepath);
				tx1->operations.push_back(op);
				tx1->tx_time = fc::time_point_sec(fc::time_point::now());
				contract1_addr = op.calculate_contract_id();

				chain->evaluate_transaction(tx1);
				chain->accept_transaction_to_mempool(*tx1);
			}
			chain->generate_block();
			{
				auto tx = std::make_shared<transaction>();
				fc::variants arrArgs;
				fc::variant aarg;
				fc::to_variant(std::string("test,TEST,10000,100"), aarg);
				arrArgs.push_back(aarg);
				auto op = operations_helper::invoke_contract(caller_addr, contract1_addr, "init_token", arrArgs);
				tx->operations.push_back(op);
				tx->tx_time = fc::time_point_sec(fc::time_point::now());

				chain->add_breakpoint_in_last_debugger_state(contract1_addr, 141);
				chain->evaluate_transaction(tx);
				auto storages = chain->get_contract_storages(contract1_addr);

				auto localvars1 = chain->view_localvars_in_last_debugger_state();
				auto stack1 = chain->view_current_contract_stack_item_in_last_debugger_state();
				auto line1 = chain->view_current_line_number_in_last_debugger_state();
				chain->clear_breakpoints_in_last_debugger_state();

				chain->debugger_step_out();
				auto localvars2 = chain->view_localvars_in_last_debugger_state();
				auto line2 = chain->view_current_line_number_in_last_debugger_state();
				// TODO: get debugger state, debug, test step in/step out/step over/view info
				
				chain->evaluate_transaction(tx);
				chain->accept_transaction_to_mempool(*tx);
			}
			chain->generate_block();
			FC_ASSERT(chain->get_account_asset_balance(caller_addr, 0) == 123);
			FC_ASSERT(chain->get_contract_by_address(contract1_addr));
			auto state = chain->get_storage(contract1_addr, "state").as<std::string>();
			FC_ASSERT(state == "\"COMMON\"");

			const auto& chain_state = chain->get_state_json();
		}

		RpcServer rpc_server(chain.get(), 8080);
		rpc_server.start();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	catch (...) {
		auto e = std::current_exception();
		std::cerr << "some error happen" << std::endl;
	}
	return 0;
}

#endif
