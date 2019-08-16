#include <simplechain/transfer_evaluate.h>
#include <simplechain/blockchain.h>
#include <simplechain/address_helper.h>

namespace simplechain {

	static void params_assert(bool cond, const std::string& msg = "") {
		if (!cond) {
			throw uvm::core::UvmException(msg.empty() ? "params invalid" : msg);
		}
	}

	std::shared_ptr<register_account_operation_evaluator::operation_type::result_type> register_account_operation_evaluator::do_evaluate(const operation_type& op) {
		return std::make_shared<void_evaluate_result>();
	}
	std::shared_ptr<register_account_operation_evaluator::operation_type::result_type> register_account_operation_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		get_chain()->register_account(op.account_address, op.pubkey_hex);
		return result;
	}

	std::shared_ptr<min_operation_evaluator::operation_type::result_type> min_operation_evaluator::do_evaluate(const operation_type& op) {
		auto asset_item = get_chain()->get_asset(op.asset_id);
		params_assert(asset_item != nullptr);
		params_assert(op.amount > 0);
		params_assert(helper::is_valid_address(op.account_address));
		return std::make_shared<void_evaluate_result>();
	}
	std::shared_ptr<min_operation_evaluator::operation_type::result_type> min_operation_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		get_chain()->update_account_asset_balance(op.account_address, op.asset_id, op.amount);
		return result;
	}

	std::shared_ptr<transfer_operation_evaluator::operation_type::result_type> transfer_operation_evaluator::do_evaluate(const operation_type& op) {
		auto asset_item = get_chain()->get_asset(op.asset_id);
		params_assert(asset_item != nullptr);
		params_assert(op.amount > 0);
		params_assert(helper::is_valid_address(op.from_address) && !helper::is_valid_contract_address(op.from_address));
		params_assert(helper::is_valid_address(op.to_address) && !helper::is_valid_contract_address(op.to_address));
		params_assert(get_chain()->get_account_asset_balance(op.from_address, op.asset_id) >= op.amount);
		return std::make_shared<void_evaluate_result>();
	}
	std::shared_ptr<transfer_operation_evaluator::operation_type::result_type> transfer_operation_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		params_assert(get_chain()->get_account_asset_balance(op.from_address, op.asset_id) >= op.amount);
		get_chain()->update_account_asset_balance(op.to_address, op.asset_id, op.amount);
		get_chain()->update_account_asset_balance(op.from_address, op.asset_id, - int64_t(op.amount));
		return result;
	}
}
