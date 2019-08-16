#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/wallfacer_lock_balance.hpp>

namespace graphene {
	namespace chain {
		class wallfacer_lock_balance_evaluator : public evaluator<wallfacer_lock_balance_evaluator> {
		public:
			typedef wallfacer_lock_balance_operation operation_type;

			void_result do_evaluate(const wallfacer_lock_balance_operation& o);
			void_result do_apply(const wallfacer_lock_balance_operation& o);
		};
		class wallfacer_foreclose_balance_evaluator :public evaluator <wallfacer_foreclose_balance_evaluator> {
		public :
			typedef wallfacer_foreclose_balance_operation operation_type;
			void_result do_evaluate(const wallfacer_foreclose_balance_operation& o);
			void_result do_apply(const wallfacer_foreclose_balance_operation& o);
		};
		class wallfacer_update_multi_account_evaluator :public evaluator<wallfacer_update_multi_account_evaluator> {
		public:
			typedef wallfacer_update_multi_account_operation operation_type;
			void_result do_evaluate(const wallfacer_update_multi_account_operation& o);
			void_result do_apply(const wallfacer_update_multi_account_operation& o);
		};

	}
}
