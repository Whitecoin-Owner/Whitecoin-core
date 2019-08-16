#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>

// #include <crtdbg.h>

#include <iostream>
#include <string>

#include <simplechain/simplechain.h>
#include <iostream>
#include <simplechain/rpcserver.h>
#include <uvm/uvm_lutil.h>
#include <fc/crypto/hex.hpp>

using namespace simplechain;

//
//#define BOOST_TEST_MODULE SimpleGcTestCases
//#include <boost/test/unit_test.hpp>

//BOOST_AUTO_TEST_SUITE(simplechain_test_suite)

//BOOST_AUTO_TEST_CASE(rlp_test)
//{
	//using namespace uvm::util;
	//RlpEncoder encoder;
	//RlpDecoder decoder;
	//auto obj1 = RlpObject::from_string("dog");
	//const auto& encoded1 = encoder.encode(obj1);
	//auto obj2 = RlpObject::from_list({ RlpObject::from_string("cat") , RlpObject::from_string("dog") });
	//auto encoded2 = encoder.encode(obj2);
	//auto obj3 = RlpObject::from_string("");
	//auto encoded3 = encoder.encode(obj3);
	//auto obj4 = RlpObject::from_list({});
	//auto encoded4 = encoder.encode(obj4);
	//auto obj5 = RlpObject::from_int32(15);
	//auto encoded5 = encoder.encode(obj5);
	//auto obj6 = RlpObject::from_int32(1024);
	//auto encoded6 = encoder.encode(obj6);
	//auto obj7 = RlpObject::from_list({ RlpObject::from_list({}), RlpObject::from_list({ RlpObject::from_list({}) }),
	//	RlpObject::from_list({ RlpObject::from_list({}), RlpObject::from_list({ RlpObject::from_list({}) }) }) });
	//auto encoded7 = encoder.encode(obj7);
	//auto obj8 = RlpObject::from_string("Lorem ipsum dolor sit amet, consectetur adipisicing elit");
	//auto encoded8 = encoder.encode(obj8);
	//// test decoder
	//auto d1 = decoder.decode(encoded1)->to_string(); // "dog"
	//auto d2 = decoder.decode(encoded2); // ["cat", "dog"]
	//auto d3 = decoder.decode(encoded3)->to_string(); // ""
	//auto d4 = decoder.decode(encoded4); // []
	//auto d5 = decoder.decode(encoded5)->to_int32(); // 15
	//auto d6 = decoder.decode(encoded6)->to_int32(); // 1024
	//auto d7 = decoder.decode(encoded7); // [ [], [[]], [ [], [[]] ] ]
	//auto d8 = decoder.decode(encoded8)->to_string(); // "Lorem ipsum dolor sit amet, consectetur adipisicing elit"

	//BOOST_CHECK(true);
//}

// BOOST_AUTO_TEST_CASE(simple_chain_demo_test)
void test2()
{
	std::cout << "Hello, simplechain based on uvm" << std::endl;
	try {
		auto chain = std::make_shared<simplechain::blockchain>();

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
		printf("");
		{
			auto tx1 = std::make_shared<transaction>();

			std::string contract1_gpc_filepath("../test/test_contracts/token.gpc");
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

			chain->evaluate_transaction(tx);
			chain->accept_transaction_to_mempool(*tx);
		}
		chain->generate_block();
		FC_ASSERT(chain->get_account_asset_balance(caller_addr, 0) == 123);
		FC_ASSERT(chain->get_contract_by_address(contract1_addr));
		auto state = chain->get_storage(contract1_addr, "state").as<std::string>();
		FC_ASSERT(state == "\"COMMON\"");

		const auto& chain_state = chain->get_state_json();
		//BOOST_CHECK(true);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		//BOOST_CHECK(false);
	}

	_CrtDumpMemoryLeaks();
}

//BOOST_AUTO_TEST_SUITE_END()

#ifdef RUN_BOOST_TESTS

int main(int argc, char* argv[])
{
	//auto res = ::boost::unit_test::unit_test_main(&init_unit_test_suite, argc, argv);
	test2();
	// _CrtDumpMemoryLeaks();
	return 0; // res;
}

#endif
