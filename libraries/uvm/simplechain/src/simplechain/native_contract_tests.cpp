#include <simplechain/native_contract_tests.h>

namespace simplechain {
	using namespace std;

	void test_token_native_contract() {
		try {
			cout << "start test_token_native_contract" << endl;
			auto chain = std::make_shared<simplechain::blockchain>();
			std::string caller_addr = std::string(SIMPLECHAIN_ADDRESS_PREFIX) + "caller1";
			std::string caller2_addr = std::string(SIMPLECHAIN_ADDRESS_PREFIX) + "caller2";

			auto tx1 = std::make_shared<transaction>();
			auto op1 = operations_helper::create_native_contract(caller_addr, "token");
			tx1->operations.push_back(op1);
			tx1->tx_time = fc::time_point_sec(fc::time_point::now());
			auto contract1_addr = op1.calculate_contract_id();
			cout << "to evaluate native create contract" << endl;
			chain->evaluate_transaction(tx1);
			chain->accept_transaction_to_mempool(*tx1);
			chain->generate_block();

			cout << "token contract: " << contract1_addr << endl;

			auto tx2 = std::make_shared<transaction>();

			fc::variants arrArgs;
			fc::variant aarg;
			fc::to_variant(std::string("test,TEST,100000000,10"), aarg);
			arrArgs.push_back(aarg);
			tx2->operations.push_back(operations_helper::invoke_contract(caller_addr, contract1_addr, "init_token", arrArgs));
			tx2->tx_time = fc::time_point_sec(fc::time_point::now());
			chain->accept_transaction_to_mempool(*tx2);
			chain->generate_block();

			cout << "token contract inited" << endl;
			size_t count = 10000;
			for (size_t i = 0; i < count; i++) {
				auto tx3 = std::make_shared<transaction>();
				fc::variants arrArgs;
				fc::variant aarg;
				fc::to_variant(caller2_addr + "," + std::to_string(i + 1), aarg);
				arrArgs.push_back(aarg);
				tx3->operations.push_back(operations_helper::invoke_contract(caller_addr, contract1_addr, "transfer", arrArgs));
				tx3->tx_time = fc::time_point_sec(fc::time_point::now());
				chain->accept_transaction_to_mempool(*tx3);
			}
			cout << "start generate block" << endl;
			auto start_time = fc::time_point_sec(fc::time_point::now());
			chain->generate_block();
			
			auto end_time = fc::time_point_sec(fc::time_point::now());
			auto using_time = end_time - start_time;
			cout << "generate block for " << count << " transfers using time " << using_time.to_seconds() << " seconds" << endl;
			auto block = chain->get_block_by_number(chain->head_block_number()-1);
			cout << "block txs count: " << block->txs.size() << endl;
		}
		catch (const std::exception& e) {
			cout << "error " << e.what() << endl;
		}
	}
}
