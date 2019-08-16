#include <native_contract/native_uniswap_contract.h>
#include <boost/algorithm/string.hpp>
#include <jsondiff/jsondiff.h>
#include <cbor_diff/cbor_diff.h>
#include <uvm/uvm_lutil.h>
#include <safenumber/safenumber.h>

namespace uvm {
	namespace contract {
		using namespace cbor_diff;
		using namespace cbor;
		using namespace uvm::contract;

#define MAX_ASSET_AMOUNT  4000000000000000

		// token contract
		std::string uniswap_native_contract::contract_key() const
		{
			return uniswap_native_contract::native_contract_key();
		}

		std::set<std::string> uniswap_native_contract::apis() const {
			return { "init", "init_token", "transfer", "transferFrom", "balanceOf", "approve", "approvedBalanceFrom", "allApprovedFromUser", "state", "supply", "totalSupply", "precision", "tokenName", "tokenSymbol","addLiquidity",
			"removeLiquidity","on_deposit_asset","init_config","withraw","setMinAddAmount","caculateExchangeAmount","getInfo","balanceOfAsset","getUserRemoveableLiquidity","caculatePoolShareByToken" };
		}
		std::set<std::string> uniswap_native_contract::offline_apis() const {
			return { "balanceOf", "approvedBalanceFrom", "allApprovedFromUser", "state", "supply", "totalSupply", "precision", "tokenName", "tokenSymbol",
			"caculateExchangeAmount","getInfo","balanceOfAsset","getUserRemoveableLiquidity","caculatePoolShareByToken" };
		}
		std::set<std::string> uniswap_native_contract::events() const {
			return { "Inited", "Transfer", "Approved","SetMinAddAmount","Exchanged","Withdrawed",
				"Deposited","LiquidityAdded","LiquidityTokenMinted","LiquidityRemoved","LiquidityTokenDestoryed" };
		}

		static const std::string not_inited_state_of_contract = "NOT_INITED";
		static const std::string common_state_of_contract = "COMMON";

		static bool is_numeric(std::string number)
		{
			char* end = 0;
			std::strtod(number.c_str(), &end);
			return end != 0 && *end == 0;
		}

		static bool is_integral(std::string number)
		{
			return is_numeric(number.c_str()) && std::strchr(number.c_str(), '.') == 0;
		}

		void uniswap_native_contract::init_api(const std::string& api_name, const std::string& api_arg)
		{
			this->token_native_contract::init_api(api_name, api_arg);
			set_current_contract_storage("fee_rate", CborObject::from_string("0.0")); 
			set_current_contract_storage("asset_1_symbol", CborObject::from_string("")); 
			set_current_contract_storage("asset_2_symbol", CborObject::from_string(""));
			set_current_contract_storage("asset_1_pool_amount", CborObject::from_int(0));
			set_current_contract_storage("asset_2_pool_amount", CborObject::from_int(0));
			set_current_contract_storage("min_asset1_amount", CborObject::from_int(0));
			set_current_contract_storage("min_asset2_amount", CborObject::from_int(0));
			return;
		}

		static int64_t getInputPrice(int64_t input_amount, int64_t input_reserve, int64_t output_reserve) {
			const auto& a = safe_number_create(input_reserve);
			const auto& b = safe_number_create(output_reserve);
			const auto& x = safe_number_create(input_amount);
			const auto& n = safe_number_multiply(x, b);
			int64_t r = safe_number_to_int64(safe_number_div(n, safe_number_add(a, x)));
			return r;
		}

		//设置增加流动性时最小增加数量
		//min_asset1_amount,min_asset2_amount
		void uniswap_native_contract::setMinAddAmount_api(const std::string& api_name, const std::string& api_arg) {
			check_admin();
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common");

			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 2)
				throw_error("argument format error, need format: min_asset1_amount,min_asset2_amount");
			std::string min_amount1 = parsed_args[0];
			std::string min_amount2 = parsed_args[1];

			if (!is_integral(min_amount1) || !is_integral(min_amount2))
				throw_error("argument format error, need format: min_asset1_amount,min_asset2_amount");

			int64_t min_asset1_amount = std::stoll(min_amount1);
			int64_t min_asset2_amount = std::stoll(min_amount2);

			if (min_asset1_amount <= 0 || min_asset2_amount <= 0)
				throw_error("argument format error, min_asset_amount to add liquidity must be positive integer");

			set_current_contract_storage("min_asset1_amount", CborObject::from_int(min_asset1_amount));
			set_current_contract_storage("min_asset2_amount", CborObject::from_int(min_asset2_amount));

			emit_event("SetMinAddAmount", api_arg);
		}

		//asset1,asset2,min_asset1_amount,min_asset2_amount,fee_rate,token_name,token_symbol
		//推荐将价值低的资产作为asset1，初始添加流动性时会将asset1_amount*1000作为token数量
		void uniswap_native_contract::init_config_api(const std::string& api_name, const std::string& api_arg) {
			check_admin();
			if (get_storage_state() != not_inited_state_of_contract)
				throw_error("this contract inited before");
			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 7)
				throw_error("argument format error, need format: asset1,asset2,min_asset1_amount,min_asset2_amount,fee_rate,token_name,token_symbol");
			std::string asset1 = parsed_args[0];
			boost::trim(asset1);
			std::string asset2 = parsed_args[1];
			boost::trim(asset2);
			if (asset1.empty() || asset2.empty())
				throw_error("argument format error, need format: asset1,asset2,min_asset1_amount,min_asset2_amount,fee_rate,token_name,token_symbol");
			if (asset1==asset2)
				throw_error("asset1 is same with asset2");
			std::string min_amount1 = parsed_args[2];
			std::string min_amount2 = parsed_args[3];
			std::string feestr = parsed_args[4];
			if (!is_integral(min_amount1)|| !is_integral(min_amount2) || !is_numeric(feestr))
				throw_error("argument format error, need format: asset1,asset2,min_asset1_amount,min_asset2_amount,fee_rate,token_name,token_symbol");

			int64_t min_asset1_amount = std::stoll(min_amount1);
			int64_t min_asset2_amount = std::stoll(min_amount2);
			char* end = 0;
			
			const auto& feeRate = safe_number_create(feestr);
			if (min_asset1_amount <= 0 || min_asset2_amount <= 0)
				throw_error("argument format error, min_asset_amount to add liquidity must be positive integer");
			if (safe_number_lt(feeRate, safe_number_create(0))|| safe_number_gte(feeRate, safe_number_create(1)))
				throw_error("argument format error, fee rate must be >=0 and < 1");

			std::string token_name = parsed_args[5];
			boost::trim(token_name);
			std::string token_symbol = parsed_args[6];
			boost::trim(token_symbol);
			if (token_name.empty() || token_symbol.empty())
				throw_error("argument format error, need format: asset1,asset2,min_asset1_amount,min_asset2_amount,fee_rate,token_name,token_symbol");

			set_current_contract_storage("fee_rate", CborObject::from_string(feestr)); 
			set_current_contract_storage("asset_1_symbol", CborObject::from_string(asset1));
			set_current_contract_storage("asset_2_symbol", CborObject::from_string(asset2));
			set_current_contract_storage("min_asset1_amount", CborObject::from_int(min_asset1_amount));
			set_current_contract_storage("min_asset2_amount", CborObject::from_int(min_asset2_amount));
			set_current_contract_storage("state", CborObject::from_string(common_state_of_contract));

			set_current_contract_storage("name", CborObject::from_string(token_name));
			set_current_contract_storage("symbol", CborObject::from_string(token_symbol));
			set_current_contract_storage("precision", CborObject::from_int(1));

			jsondiff::JsonObject event_arg;
			event_arg["fee_rate"] = feestr;
			event_arg["symbol"] = token_symbol;
			event_arg["precision"] = 1;
			event_arg["name"] = token_name;
			event_arg["state"] = common_state_of_contract;
			event_arg["supply"] = 0;
			event_arg["asset_1_symbol"] = asset1;
			event_arg["asset_2_symbol"] = asset2;
			event_arg["min_asset1_amount"] = min_asset1_amount;
			event_arg["min_asset2_amount"] = min_asset2_amount;
			event_arg["state"] = common_state_of_contract;
			emit_event("Inited", uvm::util::json_ordered_dumps(event_arg));
		}

		//add_asset1_amount,max_add_asset2_amount,expired_blocknum
		void uniswap_native_contract::addLiquidity_api(const std::string& api_name, const std::string& api_arg) {
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");
			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 3)
				throw_error("argument format error, need format: add_asset1_amount,max_add_asset2_amount,expired_blocknum");
			
			std::string add_asset1_amount_str = parsed_args[0];
			std::string max_add_asset2_amount_str = parsed_args[1];
			std::string expired_blocknum_str = parsed_args[2];

			if (!is_integral(add_asset1_amount_str) || !is_integral(max_add_asset2_amount_str) || !is_integral(expired_blocknum_str))
				throw_error("argument format error, need format: add_asset1_amount,max_add_asset2_amount,expired_blocknum");

			int64_t add_asset1_amount = std::stoll(add_asset1_amount_str);
			int64_t max_add_asset2_amount = std::stoll(max_add_asset2_amount_str);
			int64_t expired_blocknum = std::stoll(expired_blocknum_str);

			if (add_asset1_amount <= 0 || max_add_asset2_amount <= 0 )
				throw_error("argument format error, add_asset_amount to add liquidity must be positive integer");
			if (expired_blocknum <= 0)
				throw_error("argument format error, expired_blocknum must be positive integer");
			auto head_blocknum = head_block_num();
			if(expired_blocknum <= head_blocknum)
				throw_error("expired blocknum:"+ expired_blocknum_str);

			if (add_asset1_amount < get_int_current_contract_storage("min_asset1_amount")) {
				throw_error("add_asset1_amount must >= min_asset1_amount");
			}
			if (max_add_asset2_amount < get_int_current_contract_storage("min_asset2_amount")) {
				throw_error("max_add_asset2_amount must >= min_asset2_amount");
			}

			//check pool
			int64_t asset_1_pool_amount = get_int_current_contract_storage("asset_1_pool_amount");
			int64_t asset_2_pool_amount = get_int_current_contract_storage("asset_2_pool_amount");
			
			const auto& a = safe_number_create(asset_1_pool_amount);
			const auto& b = safe_number_create(asset_2_pool_amount);
			const auto& x = safe_number_create(add_asset1_amount);
			int64_t caculate_asset2_amount = max_add_asset2_amount;
			if (asset_1_pool_amount != 0 && asset_2_pool_amount != 0) {
				const auto& temp = safe_number_div(safe_number_multiply(x, b), a);
				caculate_asset2_amount = safe_number_to_int64(temp);
				if (safe_number_ne(safe_number_create(caculate_asset2_amount), temp))
					caculate_asset2_amount++;
				if (caculate_asset2_amount > max_add_asset2_amount) {
					throw_error("caculate_asset2_amount > max_add_asset2_amount");
				}
			}

			//check balance
			auto from_address = get_from_address();
			const auto& asset1 = get_string_current_contract_storage("asset_1_symbol");
			const auto& asset2 = get_string_current_contract_storage("asset_2_symbol");

			auto user_asset1_balance_cbor = current_fast_map_get(from_address, asset1);
			if (!user_asset1_balance_cbor->is_integer()) {
				throw_error("no balance of "+ asset1);
			}
			auto user_asset2_balance_cbor = current_fast_map_get(from_address, asset2);
			if (!user_asset2_balance_cbor->is_integer()) {
				throw_error("no balance of " + asset2);
			}

			int64_t asset1_balance = user_asset1_balance_cbor->force_as_int();
			int64_t asset2_balance = user_asset2_balance_cbor->force_as_int();

			if ((add_asset1_amount > asset1_balance) || (caculate_asset2_amount > asset2_balance)) {
				throw_error("not enough balance");
			}
			asset1_balance = asset1_balance - add_asset1_amount;
			asset2_balance = asset2_balance - caculate_asset2_amount;

			//sub  balances
			current_fast_map_set(from_address, asset1, CborObject::from_int(asset1_balance));
			current_fast_map_set(from_address, asset2, CborObject::from_int(asset2_balance));
			
			// mint token
			int64_t supply = get_int_current_contract_storage("supply");
			
			int64_t token_amount = 0;
			if (asset_1_pool_amount == 0 && asset_2_pool_amount == 0 && supply==0) {
				token_amount = add_asset1_amount *1000;
			}
			else if (asset_1_pool_amount != 0 && asset_2_pool_amount != 0 && supply != 0) {
				const auto& temp = safe_number_div(safe_number_multiply(x, safe_number_create(supply)), a);
				token_amount = safe_number_to_int64(temp);
			}
			else {
				throw_error("internal error, asset_1_pool_amount,asset_2_pool_amount,supply not unified 0");
			}

			if (token_amount <= 0) {
				throw_error("get token_amount is 0");
			}

			asset_1_pool_amount += add_asset1_amount;
			asset_2_pool_amount += caculate_asset2_amount;
			if (asset_1_pool_amount > MAX_ASSET_AMOUNT) {
				throw_error("EXCEED MAX ASSET AMOUNT");
			}
			set_current_contract_storage("asset_1_pool_amount", CborObject::from_int(asset_1_pool_amount));
			set_current_contract_storage("asset_2_pool_amount", CborObject::from_int(asset_2_pool_amount));
			set_current_contract_storage("supply", CborObject::from_int(supply + token_amount));

			auto cbor_token_balance = current_fast_map_get("users", from_address);
			int64_t token_balance = 0;
			if (cbor_token_balance->is_integer()) {
				token_balance = cbor_token_balance->force_as_int();
			}
			current_fast_map_set("users", from_address, CborObject::from_int(token_balance+token_amount));
			
			jsondiff::JsonObject event_arg;
			event_arg[asset1] = add_asset1_amount;
			event_arg[asset2] = caculate_asset2_amount;
			emit_event("LiquidityAdded", uvm::util::json_ordered_dumps(event_arg));

			emit_event("LiquidityTokenMinted", std::to_string(token_amount));
			set_api_result(std::to_string(token_amount));

			jsondiff::JsonObject event_arg2;
			event_arg2["from"] = "";
			event_arg2["to"] = from_address;
			event_arg2["amount"] = token_amount;
			emit_event("Transfer", uvm::util::json_ordered_dumps(event_arg2));

		}

		//destory_token_amount,min_remove_asset1_amount,min_remove_asset2_amount,expired_blocknum
		void uniswap_native_contract::removeLiquidity_api(const std::string& api_name, const std::string& api_arg) {
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");
			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 4)
				throw_error("argument format error, need format: destory_token_amount,min_remove_asset1_amount,min_remove_asset2_amount,expired_blocknum");

			std::string destory_token_amount_str = parsed_args[0];
			std::string min_remove_asset1_amount_str = parsed_args[1];
			std::string min_remove_asset2_amount_str = parsed_args[2];
			std::string expired_blocknum_str = parsed_args[3];

			if (!is_integral(min_remove_asset1_amount_str) || !is_integral(min_remove_asset2_amount_str) || !is_integral(destory_token_amount_str) || !is_integral(expired_blocknum_str))
				throw_error("argument format error, need format: destory_token_amount,min_remove_asset1_amount,min_remove_asset2_amount,expired_blocknum");

			int64_t destory_token_amount = std::stoll(destory_token_amount_str);
			int64_t min_remove_asset1_amount = std::stoll(min_remove_asset1_amount_str);
			int64_t min_remove_asset2_amount = std::stoll(min_remove_asset2_amount_str);
			int64_t expired_blocknum = std::stoll(expired_blocknum_str);

			if (min_remove_asset1_amount < 0 || min_remove_asset2_amount < 0 || destory_token_amount <= 0 || expired_blocknum <= 0)
				throw_error("argument format error, input args must be positive integers");
			
			auto head_blocknum = head_block_num();
			if (expired_blocknum <= head_blocknum)
				throw_error("expired blocknum:" + expired_blocknum_str);

			//check liquidity token 
			auto from_address = get_from_address();
			auto cbor_token_balance = current_fast_map_get("users", from_address);
			int64_t token_balance = 0;
			if (cbor_token_balance->is_integer()) {
				token_balance = cbor_token_balance->force_as_int();
			}
			if (destory_token_amount > token_balance) {
				throw_error("you have not enough liquidity to remove");
			}

			//check pool
			int64_t asset_1_pool_amount = get_int_current_contract_storage("asset_1_pool_amount");
			int64_t asset_2_pool_amount = get_int_current_contract_storage("asset_2_pool_amount");
			int64_t supply = get_int_current_contract_storage("supply");
			if (asset_1_pool_amount <= 0 || supply<=0 || asset_2_pool_amount<= 0) {
				throw_error("pool is empty");
			}
			if ((min_remove_asset1_amount > asset_1_pool_amount) || (min_remove_asset2_amount > asset_2_pool_amount)) {
				throw_error("wrong remove_asset_amount");
			}

			//calulate
			int64_t caculate_asset1_amount = 0;
			int64_t caculate_asset2_amount = 0;
			if (supply == destory_token_amount) {
				caculate_asset1_amount = asset_1_pool_amount;
				caculate_asset2_amount = asset_2_pool_amount;
			}
			else {
				const auto& a = safe_number_create(asset_1_pool_amount);
				const auto& b = safe_number_create(asset_2_pool_amount);
				const auto& s = safe_number_create(supply);
				const auto& tk = safe_number_create(destory_token_amount);
				const auto& quota = safe_number_div(tk, s);
				caculate_asset1_amount = safe_number_to_int64(safe_number_multiply(quota, a));
				caculate_asset2_amount = safe_number_to_int64(safe_number_multiply(quota, b));
			}
			
			if (caculate_asset1_amount < min_remove_asset1_amount) {
				throw_error("caculate_asset1_amount < min_remove_asset1_amount");
			}
			if (caculate_asset2_amount < min_remove_asset2_amount) {
				throw_error("caculate_asset2_amount < min_remove_asset2_amount");
			}

			//add balance
			const auto& asset1 = get_string_current_contract_storage("asset_1_symbol");
			const auto& asset2 = get_string_current_contract_storage("asset_2_symbol");

			auto user_asset1_balance_cbor = current_fast_map_get(from_address, asset1);
			int64_t orig_asset1_balance = 0;
			if (user_asset1_balance_cbor->is_integer()) {
				orig_asset1_balance = user_asset1_balance_cbor->force_as_int();
			}
			auto user_asset2_balance_cbor = current_fast_map_get(from_address, asset2);
			int64_t orig_asset2_balance = 0;
			if (user_asset2_balance_cbor->is_integer()) {
				orig_asset2_balance = user_asset1_balance_cbor->force_as_int();
			}
			current_fast_map_set(from_address, asset1, CborObject::from_int(orig_asset1_balance + caculate_asset1_amount));
			current_fast_map_set(from_address, asset2, CborObject::from_int(orig_asset2_balance + caculate_asset2_amount));
			
			// sub supply, sub token ,sub pool amount
			asset_1_pool_amount -= caculate_asset1_amount;
			asset_2_pool_amount -= caculate_asset2_amount;
			supply -= destory_token_amount;
			if (!(asset_1_pool_amount == 0 && asset_2_pool_amount == 0 && supply == 0)) {
				if (!(asset_1_pool_amount != 0 && asset_2_pool_amount != 0 && supply != 0)) {
					throw_error("internal error, pool 0 when supply is not 0");
				}
			}
			set_current_contract_storage("asset_1_pool_amount", CborObject::from_int(asset_1_pool_amount));
			set_current_contract_storage("asset_2_pool_amount", CborObject::from_int(asset_2_pool_amount));
			set_current_contract_storage("supply", CborObject::from_int(supply));
			current_fast_map_set("users", from_address, CborObject::from_int(token_balance - destory_token_amount));

			jsondiff::JsonObject event_arg;
			event_arg[asset1] = caculate_asset1_amount;
			event_arg[asset2] = caculate_asset2_amount;
			emit_event("LiquidityRemoved", uvm::util::json_ordered_dumps(event_arg));

			emit_event("LiquidityTokenDestoryed", std::to_string(destory_token_amount));
		}

		//want_buy_asset_symbol,min_want_buy_asset_amount,expired_blocknum
		void uniswap_native_contract::on_deposit_asset_api(const std::string& api_name, const std::string& api_args)
		{
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");
			
			auto args = fc::json::from_string(api_args);
			if (!args.is_object()) {
				throw_error("args not map");
			}

			auto arginfo = args.as<fc::mutable_variant_object>();
			int64_t amount = arginfo["num"].as_int64();
			auto symbol = arginfo["symbol"].as_string();
			auto param = arginfo["param"].as_string();
			if (amount <= 0) {
				throw_error("amount must > 0");
			}
			const auto& from_address = get_from_address();

			const auto& asset1 = get_string_current_contract_storage("asset_1_symbol");
			const auto& asset2 = get_string_current_contract_storage("asset_2_symbol");
			if (symbol != asset1 && symbol != asset2) {
				throw_error("not allowed deposit asset:"+ symbol);
			}

			if (param.empty()) { // deposit
				auto balance_cbor = current_fast_map_get(from_address, symbol);
				int64_t user_balance = 0;
				if (balance_cbor->is_integer()) {
					user_balance = balance_cbor->force_as_int();
				}
				user_balance += amount;
				current_fast_map_set(from_address, symbol, CborObject::from_int(user_balance));
				
				jsondiff::JsonObject event_arg;
				event_arg["from_address"] = from_address;
				event_arg["symbol"] = symbol;
				event_arg["amount"] = amount;
				emit_event("Deposited", uvm::util::json_ordered_dumps(event_arg));
			}
			else { //exchange
				std::vector<std::string> parsed_args;
				boost::split(parsed_args, param, [](char c) {return c == ','; });
				if (parsed_args.size() != 3)
					throw_error("argument format error, need format: want_buy_asset_symbol,min_want_buy_asset_amount,expired_blocknum");

				std::string want_buy_asset_symbol = parsed_args[0];
				std::string want_buy_asset_amount_str = parsed_args[1];
				std::string expired_blocknum_str = parsed_args[2];

				if (!is_integral(want_buy_asset_amount_str)|| !is_integral(expired_blocknum_str))
					throw_error("argument format error, need format: want_buy_asset_symbol,min_want_buy_asset_amount,expired_blocknum");

				int64_t want_buy_asset_amount = std::stoll(want_buy_asset_amount_str);
				int64_t expired_blocknum = std::stoll(expired_blocknum_str);

				if (want_buy_asset_amount <= 0) {
					throw_error("want_buy_asset_amount must > 0");
				}
				if (expired_blocknum <= 0) {
					throw_error("expired_blocknum must > 0");
				}

				auto head_blocknum = head_block_num();
				if (expired_blocknum <= head_blocknum)
					throw_error("expired blocknum:" + expired_blocknum_str);

				if (want_buy_asset_symbol != asset1 && want_buy_asset_symbol != asset2) {
					throw_error("not support buy asset:" + want_buy_asset_symbol);
				}
				if (symbol != asset1 && symbol != asset2) {
					throw_error("not support sell asset:" + symbol);
				}
				if (want_buy_asset_symbol == symbol) {
					throw_error("deposit asset and want_buy_asset_symbol is same");
				}

				int64_t asset_1_pool_amount = get_int_current_contract_storage("asset_1_pool_amount");
				int64_t asset_2_pool_amount = get_int_current_contract_storage("asset_2_pool_amount");
				if (asset_1_pool_amount <= 0 || asset_2_pool_amount <= 0) {
					throw_error("pool is empty");
				}
				
				const auto& feeRate = get_string_current_contract_storage("fee_rate");
				const auto& nFeeRate = safe_number_create(feeRate);
				const auto& temp = safe_number_multiply(safe_number_create(amount), nFeeRate);
				int64_t fee = safe_number_to_int64(temp);
				if (safe_number_ne(safe_number_create(fee), temp))
					fee++;
				if (fee <= 0) {
					fee = 1;
				}
				if (amount <= fee) {
					throw_error("can't get any");
					return;
				}

				jsondiff::JsonObject event_arg;
				int64_t get_asset_amount = 0;
				if (symbol == asset1) {
					if (want_buy_asset_amount >= asset_2_pool_amount) {
						throw_error("pool amount not enough");
					}
					get_asset_amount = getInputPrice(amount-fee, asset_1_pool_amount, asset_2_pool_amount);
					if (get_asset_amount <= 0) {
						throw_error("get_asset_amount <= 0");
					}
					if (want_buy_asset_amount > get_asset_amount) {
						throw_error("can't get what you want amount");
					}
					asset_1_pool_amount += amount;
					asset_2_pool_amount -= get_asset_amount;
					
					event_arg["buy_asset"] = asset2;
				}
				else {
					if (want_buy_asset_amount >= asset_1_pool_amount) {
						throw_error("pool amount not enough");
					}
					get_asset_amount = getInputPrice(amount-fee, asset_2_pool_amount, asset_1_pool_amount);
					if (get_asset_amount <= 0) {
						throw_error("get_asset_amount <= 0");
					}
					if (want_buy_asset_amount > get_asset_amount) {
						throw_error("can't get what you want amount");
					}
					asset_2_pool_amount += amount;
					asset_1_pool_amount -= get_asset_amount;
					event_arg["buy_asset"] = asset1;
					
				}
				if (asset_1_pool_amount <= 0 || asset_2_pool_amount <= 0) {
					throw_error("caculate internal error");
				}
				set_current_contract_storage("asset_1_pool_amount", CborObject::from_int(asset_1_pool_amount));
				set_current_contract_storage("asset_2_pool_amount", CborObject::from_int(asset_2_pool_amount));

				current_transfer_to_address(from_address, want_buy_asset_symbol, get_asset_amount);

				event_arg["addr"] = from_address;
				event_arg["fee"] = fee; // fee symbol ->sell_asset
				event_arg["sell_asset"] = symbol;
				event_arg["sell_amount"] = amount;
				event_arg["buy_amount"] = get_asset_amount;
				emit_event("Exchanged", uvm::util::json_ordered_dumps(event_arg));
				set_api_result(std::to_string(get_asset_amount));
			}
		}

		//args:address
		void uniswap_native_contract::getUserRemoveableLiquidity_api(const std::string& api_name, const std::string& api_arg) {
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");
			
			const auto& addr = api_arg;

			auto cbor_token_balance = current_fast_map_get("users", addr);
			int64_t token_balance = 0;
			if (cbor_token_balance->is_integer()) {
				token_balance = cbor_token_balance->force_as_int();
			}

			jsondiff::JsonObject result;
			const auto& asset1 = get_string_current_contract_storage("asset_1_symbol");
			const auto& asset2 = get_string_current_contract_storage("asset_2_symbol");
			if (token_balance <= 0) {
				result[asset1] = 0;
				result[asset2] = 0;
			}
			else {
				int64_t asset_1_pool_amount = get_int_current_contract_storage("asset_1_pool_amount");
				int64_t asset_2_pool_amount = get_int_current_contract_storage("asset_2_pool_amount");
				int64_t supply = get_int_current_contract_storage("supply");

				if (token_balance == supply) {
					result[asset1] = asset_1_pool_amount;
					result[asset2] = asset_2_pool_amount;
				}
				else {
					const auto& a = safe_number_create(asset_1_pool_amount);
					const auto& b = safe_number_create(asset_2_pool_amount);
					const auto& quota = safe_number_div(safe_number_create(token_balance), safe_number_create(supply));

					result[asset1] = safe_number_to_int64(safe_number_multiply(quota, a));
					result[asset2] = safe_number_to_int64(safe_number_multiply(quota, b));
				}
			}

			const auto& r = uvm::util::json_ordered_dumps(result);
			set_api_result(r);
		}

		//args:tokenAmount
		void uniswap_native_contract::caculatePoolShareByToken_api(const std::string& api_name, const std::string& api_arg) {
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");

			const auto& asset1 = get_string_current_contract_storage("asset_1_symbol");
			const auto& asset2 = get_string_current_contract_storage("asset_2_symbol");
			int64_t asset_1_pool_amount = get_int_current_contract_storage("asset_1_pool_amount");
			int64_t asset_2_pool_amount = get_int_current_contract_storage("asset_2_pool_amount");
			int64_t supply = get_int_current_contract_storage("supply");

			if (!is_integral(api_arg)) {
				throw_error("input arg must be integer");
			}
			int64_t tokenAmount = std::stoll(api_arg);
			if (tokenAmount <= 0) {
				throw_error("input arg must be positive integer");
			}
			jsondiff::JsonObject result;
			if (tokenAmount == supply) {
				result[asset1] = asset_1_pool_amount;
				result[asset2] = asset_2_pool_amount;
			}
			else if (tokenAmount > supply) {
				throw_error("input tokenAmount must <= supply");
			}
			else {
				const auto& a = safe_number_create(asset_1_pool_amount);
				const auto& b = safe_number_create(asset_2_pool_amount);
				const auto& quota = safe_number_div(safe_number_create(tokenAmount), safe_number_create(supply));

				result[asset1] = safe_number_to_int64(safe_number_multiply(quota, a));
				result[asset2] = safe_number_to_int64(safe_number_multiply(quota, b));
			}

			const auto& r = uvm::util::json_ordered_dumps(result);
			set_api_result(r);
		}
				
		//args:address,symbol
		void uniswap_native_contract::balanceOfAsset_api(const std::string& api_name, const std::string& api_arg) {
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");

			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 2)
				throw_error("argument format error, need format: address,symbol");

			const auto& addr = parsed_args[0];
			const auto& symbol = parsed_args[1];

			auto balance = current_fast_map_get(addr, symbol);
			int64_t bal = 0;
			if (balance->is_integer()) {
				bal = balance->force_as_int();
			}
			set_api_result(std::to_string(bal));
		}

		//args:amount,symbol
		void uniswap_native_contract::withdraw_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");

			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 2)
				throw_error("argument format error, need format: amount,symbol");

			if (!is_integral(parsed_args[0]))
				throw_error("argument format error, amount must be integral");

			int64_t amount = fc::to_int64(parsed_args[0]);

			if (amount <= 0) {
				throw_error("amount must > 0");
			}
			const auto& symbol = parsed_args[1];
			if (symbol.empty()) {
				throw_error("symbol is empty");
			}
			const auto& from_address = caller_address_string();

			auto balance = current_fast_map_get(from_address, symbol);
			int64_t bal = 0;
			if (!balance->is_integer()) {
				throw_error("no balance to withdraw");
			}
			else {
				bal = balance->force_as_int();
			}
			if (amount > bal) {
				throw_error("not enough balance to withdraw");
			}
			int64_t newBalance = bal - amount;
			if (newBalance == 0) {
				current_fast_map_set(from_address, parsed_args[1], CborObject::create_null());
			}
			else {
				current_fast_map_set(from_address, parsed_args[1], CborObject::from_int(newBalance));
			}

			current_transfer_to_address(from_address, symbol, amount);
			//{"amount":35000,"realAmount":34996,"fee":4,"symbol":"BTC","to_address":"test11"}
			jsondiff::JsonObject event_arg;
			event_arg["symbol"] = symbol;
			event_arg["to_address"] = from_address;
			event_arg["amount"] = amount;
			emit_event("Withdrawed", uvm::util::json_ordered_dumps(event_arg));
		}

		//args: want_sell_asset_symbol,want_sell_asset_amount,want_buy_asset_symbol
		//return : get_amount
		void uniswap_native_contract::caculateExchangeAmount_api(const std::string& api_name, const std::string& api_arg) {
			if (get_storage_state() != common_state_of_contract)
				throw_error("state not common!");
			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 3)
				throw_error("argument format error, need format: want_sell_asset_symbol,want_sell_asset_amount,want_buy_asset_symbol");

			std::string want_sell_asset_symbol = parsed_args[0];
			std::string want_sell_asset_amount_str = parsed_args[1];
			std::string want_buy_asset_symbol = parsed_args[2];

			if (!is_integral(want_sell_asset_amount_str))
				throw_error("argument format error, need format: want_sell_asset_symbol,want_sell_asset_amount");

			int64_t want_sell_asset_amount = std::stoll(want_sell_asset_amount_str);
			if (want_sell_asset_amount <= 0) {
				throw_error("want_sell_asset_amount must > 0");
			}
			
			const auto& asset1 = get_string_current_contract_storage("asset_1_symbol");
			const auto& asset2 = get_string_current_contract_storage("asset_2_symbol");
			int64_t asset_1_pool_amount = get_int_current_contract_storage("asset_1_pool_amount");
			int64_t asset_2_pool_amount = get_int_current_contract_storage("asset_2_pool_amount");
			if (asset_1_pool_amount <= 0 || asset_2_pool_amount <= 0) {
				throw_error("pool is empty");
			}

			///
			const auto& feeRate = get_string_current_contract_storage("fee_rate");
			const auto& nFeeRate = safe_number_create(feeRate);
			const auto& temp = safe_number_multiply(safe_number_create(want_sell_asset_amount), nFeeRate);
			int64_t fee = safe_number_to_int64(temp);
			if (safe_number_ne(safe_number_create(fee), temp))
				fee++;
			if (fee <= 0) {
				fee = 1;
			}
			if (want_sell_asset_amount <= fee) {
				set_api_result(std::to_string(0));
				return;
			}
			int64_t get_asset_amount = 0;
			if (want_sell_asset_symbol == asset1 && want_buy_asset_symbol == asset2) {
				get_asset_amount = getInputPrice(want_sell_asset_amount-fee, asset_1_pool_amount, asset_2_pool_amount);
			}
			else if (want_sell_asset_symbol == asset2 && want_buy_asset_symbol == asset1)
			{
				get_asset_amount = getInputPrice(want_sell_asset_amount-fee, asset_2_pool_amount, asset_1_pool_amount);
			}
			else {
				throw_error("input asset symbol not match");
			}
			set_api_result(std::to_string(get_asset_amount));

		}


		void uniswap_native_contract::getInfo_api(const std::string& api_name, const std::string& api_arg) {
			jsondiff::JsonObject infos;
			infos["admin"] = get_current_contract_storage_cbor("admin")->as_string();
			infos["state"] = get_current_contract_storage_cbor("state")->as_string();
			infos["fee_rate"] = get_current_contract_storage_cbor("fee_rate")->as_string();
			infos["token_name"] = get_current_contract_storage_cbor("name")->as_string();
			infos["token_symbol"] = get_current_contract_storage_cbor("symbol")->as_string();
			infos["supply"] = get_current_contract_storage_cbor("supply")->force_as_int();
			infos["asset_1_symbol"] = get_current_contract_storage_cbor("asset_1_symbol")->as_string();
			infos["asset_2_symbol"] = get_current_contract_storage_cbor("asset_2_symbol")->as_string();
			infos["asset_1_pool_amount"] = get_current_contract_storage_cbor("asset_1_pool_amount")->force_as_int();
			infos["asset_2_pool_amount"] = get_current_contract_storage_cbor("asset_2_pool_amount")->force_as_int();
			infos["min_asset1_amount"] = get_current_contract_storage_cbor("min_asset1_amount")->force_as_int();
			infos["min_asset2_amount"] = get_current_contract_storage_cbor("min_asset2_amount")->force_as_int();
			const auto& r = uvm::util::json_ordered_dumps(infos);
			set_api_result(r);
		}

		

		void uniswap_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			std::map<std::string, std::function<void(const std::string&, const std::string&)>> apis = {
			{ "init", std::bind(&uniswap_native_contract::init_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "init_config", std::bind(&uniswap_native_contract::init_config_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "transfer", std::bind(&uniswap_native_contract::transfer_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "transferFrom", std::bind(&uniswap_native_contract::transfer_from_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "balanceOf", std::bind(&uniswap_native_contract::balance_of_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "approve", std::bind(&uniswap_native_contract::approve_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "approvedBalanceFrom", std::bind(&uniswap_native_contract::approved_balance_from_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "allApprovedFromUser", std::bind(&uniswap_native_contract::all_approved_from_user_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "state", std::bind(&uniswap_native_contract::state_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "supply", std::bind(&uniswap_native_contract::supply_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "totalSupply", std::bind(&uniswap_native_contract::totalSupply_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "precision", std::bind(&uniswap_native_contract::precision_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "tokenName", std::bind(&uniswap_native_contract::token_name_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "tokenSymbol", std::bind(&uniswap_native_contract::token_symbol_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "on_deposit_asset", std::bind(&uniswap_native_contract::on_deposit_asset_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "addLiquidity", std::bind(&uniswap_native_contract::addLiquidity_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "removeLiquidity", std::bind(&uniswap_native_contract::removeLiquidity_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "setMinAddAmount", std::bind(&uniswap_native_contract::setMinAddAmount_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "withdraw", std::bind(&uniswap_native_contract::withdraw_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "caculatePoolShareByToken", std::bind(&uniswap_native_contract::caculatePoolShareByToken_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "caculateExchangeAmount", std::bind(&uniswap_native_contract::caculateExchangeAmount_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "getInfo", std::bind(&uniswap_native_contract::getInfo_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "balanceOfAsset", std::bind(&uniswap_native_contract::balanceOfAsset_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "getUserRemoveableLiquidity", std::bind(&uniswap_native_contract::getUserRemoveableLiquidity_api, this, std::placeholders::_1, std::placeholders::_2) },
			};
			if (apis.find(api_name) != apis.end())
			{
				apis[api_name](api_name, api_arg);
				set_invoke_result_caller();
				add_gas(gas_count_for_api_invoke(api_name));
				return;
			}
			throw_error("api not found");
		}
	}
}
