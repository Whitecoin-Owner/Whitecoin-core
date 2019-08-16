#pragma once
#include <simplechain/transfer_operation.h>
#include <simplechain/evaluate_state.h>
#include <simplechain/evaluate_result.h>
#include <simplechain/evaluator.h>

namespace simplechain {
	class register_account_operation_evaluator : public evaluator<register_account_operation_evaluator>, public evaluate_state {
	public:
		typedef register_account_operation operation_type;
	public:
		register_account_operation_evaluator(blockchain* chain_, transaction* tx_) : evaluate_state(chain_, tx_) {}
		virtual ~register_account_operation_evaluator() {}

		virtual std::shared_ptr<operation_type::result_type> do_evaluate(const operation_type& op) final;
		virtual std::shared_ptr<operation_type::result_type> do_apply(const operation_type& op) final;
	};

	class min_operation_evaluator : public evaluator<min_operation_evaluator>, public evaluate_state {
	public:
		typedef mint_operation operation_type;
	public:
		min_operation_evaluator(blockchain* chain_, transaction* tx_) : evaluate_state(chain_, tx_) {}
		virtual ~min_operation_evaluator() {}

		virtual std::shared_ptr<operation_type::result_type> do_evaluate(const operation_type& op) final;
		virtual std::shared_ptr<operation_type::result_type> do_apply(const operation_type& op) final;
	};

	class transfer_operation_evaluator : public evaluator<transfer_operation_evaluator>, public evaluate_state {
	public:
		typedef transfer_operation operation_type;
	public:
		transfer_operation_evaluator(blockchain* chain_, transaction* tx_) : evaluate_state(chain_, tx_) {}
		virtual ~transfer_operation_evaluator() {}

		virtual std::shared_ptr<operation_type::result_type> do_evaluate(const operation_type& op) final;
		virtual std::shared_ptr<operation_type::result_type> do_apply(const operation_type& op) final;
	};
}
