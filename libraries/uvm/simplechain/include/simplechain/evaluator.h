#pragma once

#include <simplechain/config.h>
#include <simplechain/operation.h>
#include <simplechain/operations.h>
#include <simplechain/evaluate_result.h>

namespace simplechain {

	struct transaction;

	class generic_evaluator {
	public:
		virtual ~generic_evaluator() {}

		virtual std::shared_ptr<evaluate_result> evaluate(const operation& op) = 0;
		virtual std::shared_ptr<evaluate_result> apply(const operation& op) = 0;
	};

	template <typename evaluator_type>
	class evaluator : public generic_evaluator {
	public:
		// TODO: context and db
	public:
		virtual std::shared_ptr<evaluate_result> evaluate(const operation& op) {
			auto* eval = static_cast<evaluator_type*>(this);
			auto real_op = op.get<typename evaluator_type::operation_type>();
			return eval->do_evaluate(real_op);
		}
		virtual std::shared_ptr<evaluate_result> apply(const operation& op) {
			auto* eval = static_cast<evaluator_type*>(this);
			auto real_op = op.get<typename evaluator_type::operation_type>();
			return eval->do_apply(real_op);
		}
	};
}
