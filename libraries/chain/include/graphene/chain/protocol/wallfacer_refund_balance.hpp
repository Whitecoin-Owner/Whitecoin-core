#pragma once
#include <graphene/chain/protocol/base.hpp>


namespace graphene {
	namespace chain {

		struct wallfacer_refund_balance_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			address    refund_addr;
			string     txid;
			void       validate()const;
			address fee_payer()const {
				return refund_addr;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, refund_addr, 1));
			}

		};
		struct wallfacer_refund_crosschain_trx_operation:public base_operation{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			address    wallfacer_address;
			wallfacer_member_id_type wallfacer_id;
			transaction_id_type not_enough_sign_trx_id;
			address fee_payer()const {
				return wallfacer_address;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }

		};
		struct wallfacer_cancel_combine_trx_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			address    wallfacer_address;
			wallfacer_member_id_type wallfacer_id;
			transaction_id_type fail_trx_id;
			address fee_payer()const {
				return wallfacer_address;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
		};
		struct eth_cancel_fail_crosschain_trx_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			address    wallfacer_address;
			wallfacer_member_id_type wallfacer_id;
			transaction_id_type fail_transaction_id;
			address fee_payer()const {
				return wallfacer_address;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
		};
		struct wallfacer_pass_success_trx_operation:public base_operation{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			address    wallfacer_address;
			wallfacer_member_id_type wallfacer_id;
			transaction_id_type pass_transaction_id;
			address fee_payer()const {
				return wallfacer_address;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
		};
	}
}
FC_REFLECT(graphene::chain::wallfacer_refund_balance_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::wallfacer_refund_balance_operation, (fee)(refund_addr)(txid))
FC_REFLECT(graphene::chain::wallfacer_refund_crosschain_trx_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::wallfacer_refund_crosschain_trx_operation, (fee)(wallfacer_address)(wallfacer_id)(not_enough_sign_trx_id))
FC_REFLECT(graphene::chain::wallfacer_cancel_combine_trx_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::wallfacer_cancel_combine_trx_operation, (fee)(wallfacer_address)(wallfacer_id)(fail_trx_id))
FC_REFLECT(graphene::chain::eth_cancel_fail_crosschain_trx_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eth_cancel_fail_crosschain_trx_operation, (fee)(wallfacer_address)(wallfacer_id)(fail_transaction_id))
FC_REFLECT(graphene::chain::wallfacer_pass_success_trx_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::wallfacer_pass_success_trx_operation, (fee)(wallfacer_address)(wallfacer_id)(pass_transaction_id))
