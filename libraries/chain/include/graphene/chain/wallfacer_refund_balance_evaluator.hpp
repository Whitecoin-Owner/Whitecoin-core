#pragma once

#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene {
	namespace chain {

		class wallfacer_refund_balance_evaluator : public evaluator<wallfacer_refund_balance_evaluator>
		{
		public:
			typedef wallfacer_refund_balance_operation operation_type;
			void_result do_evaluate(const wallfacer_refund_balance_operation& o);
			void_result do_apply(const wallfacer_refund_balance_operation& o);
		};
		class wallfacer_refund_crosschain_trx_evaluator :public evaluator<wallfacer_refund_crosschain_trx_evaluator> {
		public:
			typedef wallfacer_refund_crosschain_trx_operation operation_type;
			void_result do_evaluate(const wallfacer_refund_crosschain_trx_operation& o);
			void_result do_apply(const wallfacer_refund_crosschain_trx_operation& o);
		};
		class wallfacer_cancel_combine_trx_evaluator :public evaluator<wallfacer_cancel_combine_trx_evaluator> {
		public:
			typedef wallfacer_cancel_combine_trx_operation operation_type;
			void_result do_evaluate(const wallfacer_cancel_combine_trx_operation& o);
			void_result do_apply(const wallfacer_cancel_combine_trx_operation& o);
		};
		class eths_cancel_unsigned_transaction_evaluator :public evaluator<eths_cancel_unsigned_transaction_evaluator> {
		public:
			typedef eths_cancel_unsigned_transaction_operation operation_type;
			void_result do_evaluate(const eths_cancel_unsigned_transaction_operation& o);
			void_result do_apply(const eths_cancel_unsigned_transaction_operation& o);
		};
		class eth_cancel_fail_crosschain_trx_evaluate :public evaluator<eth_cancel_fail_crosschain_trx_evaluate> {
		public:
			typedef eth_cancel_fail_crosschain_trx_operation operation_type;
			void_result do_evaluate(const eth_cancel_fail_crosschain_trx_operation& o);
			void_result do_apply(const eth_cancel_fail_crosschain_trx_operation& o);
		};
		class wallfacer_pass_success_trx_evaluate :public evaluator<wallfacer_pass_success_trx_evaluate> {
		public:
			typedef wallfacer_pass_success_trx_operation operation_type;
			void_result do_evaluate(const wallfacer_pass_success_trx_operation& o);
			void_result do_apply(const wallfacer_pass_success_trx_operation& o);
		};
	}
}
