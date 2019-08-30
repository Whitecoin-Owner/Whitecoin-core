#pragma once
#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/crosschain/crosschain_impl.hpp>
namespace graphene {
	namespace chain {
		struct eth_series_multi_sol_create_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			miner_id_type miner_broadcast;
			address miner_broadcast_addrss;
			wallfacer_member_id_type wallfacer_to_sign;
			std::string multi_cold_address;
			std::string multi_hot_address;
			std::string multi_account_tx_without_sign_hot;
			std::string multi_account_tx_without_sign_cold;
			std::string cold_nonce;
			std::string hot_nonce;
			std::string chain_type;
			std::string wallfacer_sign_hot_address;
			std::string wallfacer_sign_cold_address;
			address fee_payer()const {
				return miner_broadcast_addrss;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_broadcast_addrss, 1));
			}
		};
		struct eths_multi_sol_wallfacer_sign_operation : public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			transaction_id_type sol_without_sign_txid;
			wallfacer_member_id_type wallfacer_to_sign;
			address wallfacer_sign_address;
			std::string multi_cold_trxid;
			std::string multi_hot_trxid;
			std::string multi_hot_sol_wallfacer_sign;
			std::string multi_cold_sol_wallfacer_sign;
			std::string chain_type;
			address fee_payer()const {
				return wallfacer_sign_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, wallfacer_sign_address, 1));
			}
		};
		//gather record
		struct eth_multi_account_create_record_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			miner_id_type miner_broadcast;
			address miner_address;
			std::string chain_type;
			std::string multi_pubkey_type;
			graphene::crosschain::hd_trx eth_multi_account_trx;
			transaction_id_type pre_trx_id;
			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		};
		
		struct eth_seri_wallfacer_sign_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			fc::variant_object eth_wallfacer_sign_trx;
			address wallfacer_address;
			wallfacer_member_id_type wallfacer_to_sign;
			address fee_payer()const {
				return wallfacer_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, wallfacer_address, 1));
			}
		};
		struct eths_wallfacer_sign_final_operation:public base_operation{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			wallfacer_member_id_type wallfacer_to_sign;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address wallfacer_address;
			std::string chain_type;
			address fee_payer()const {
				return wallfacer_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, wallfacer_address, 1));
			}
		};
		struct eths_wallfacer_change_signer_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			wallfacer_member_id_type wallfacer_to_sign;
			std::string new_signer;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address wallfacer_address;
			std::string chain_type;
			address fee_payer()const {
				return wallfacer_address;
			}
			share_type      calculate_fee(const fee_parameters_type& k)const;
		};
		
		struct eths_wallfacer_coldhot_change_signer_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			wallfacer_member_id_type wallfacer_to_sign;
			std::string new_signer;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address wallfacer_address;
			std::string chain_type;
			address fee_payer()const {
				return wallfacer_address;
			}
			share_type      calculate_fee(const fee_parameters_type& k)const;
		};
		struct eths_cancel_unsigned_transaction_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			address    wallfacer_address;
			wallfacer_member_id_type wallfacer_id;
			transaction_id_type cancel_trx_id;
			address fee_payer()const {
				return wallfacer_address;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
		};
		struct eths_coldhot_wallfacer_sign_final_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			wallfacer_member_id_type wallfacer_to_sign;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address wallfacer_address;
			std::string chain_type;
			address fee_payer()const {
				return wallfacer_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, wallfacer_address, 1));
			}
		};
		struct wallfacer_change_acquire_trx_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
			};
			asset fee;
			address    wallfacer_address;
			wallfacer_member_id_type wallfacer_id;
			transaction_id_type change_transaction_id;
			address fee_payer()const {
				return wallfacer_address;
	}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
		};
}
}
FC_REFLECT(graphene::chain::wallfacer_change_acquire_trx_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::wallfacer_change_acquire_trx_operation, (wallfacer_address)(wallfacer_id)(change_transaction_id))
FC_REFLECT(graphene::chain::eth_seri_wallfacer_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eth_seri_wallfacer_sign_operation, (fee)(eth_wallfacer_sign_trx)(wallfacer_address)(wallfacer_to_sign))
FC_REFLECT(graphene::chain::eth_series_multi_sol_create_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eth_series_multi_sol_create_operation, (fee)(wallfacer_to_sign)(miner_broadcast)(miner_broadcast_addrss)(multi_cold_address)
																	(multi_hot_address)(multi_account_tx_without_sign_hot)(multi_account_tx_without_sign_cold)(cold_nonce)(hot_nonce)(chain_type)(wallfacer_sign_hot_address)(wallfacer_sign_cold_address))
FC_REFLECT(graphene::chain::eth_multi_account_create_record_operation::fee_parameters_type,(fee))
FC_REFLECT(graphene::chain::eth_multi_account_create_record_operation, (fee)(miner_broadcast)(miner_address)(chain_type)(multi_pubkey_type)(eth_multi_account_trx)(eth_multi_account_trx)(pre_trx_id))
FC_REFLECT(graphene::chain::eths_multi_sol_wallfacer_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_multi_sol_wallfacer_sign_operation, (fee)(sol_without_sign_txid)(wallfacer_to_sign)(wallfacer_sign_address)
																(multi_hot_sol_wallfacer_sign)(multi_cold_trxid)(multi_hot_trxid)(multi_cold_sol_wallfacer_sign)(chain_type))
FC_REFLECT(graphene::chain::eths_wallfacer_sign_final_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_wallfacer_sign_final_operation, (fee)(wallfacer_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(chain_type)(signed_crosschain_trx)(wallfacer_address))
FC_REFLECT(graphene::chain::eths_coldhot_wallfacer_sign_final_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_coldhot_wallfacer_sign_final_operation, (fee)(wallfacer_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(chain_type)(signed_crosschain_trx)(wallfacer_address))
FC_REFLECT(graphene::chain::eths_wallfacer_change_signer_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_wallfacer_change_signer_operation, (fee)(wallfacer_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(new_signer)(chain_type)(signed_crosschain_trx)(wallfacer_address))
FC_REFLECT(graphene::chain::eths_wallfacer_coldhot_change_signer_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_wallfacer_coldhot_change_signer_operation, (fee)(wallfacer_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(new_signer)(chain_type)(signed_crosschain_trx)(wallfacer_address))

FC_REFLECT(graphene::chain::eths_cancel_unsigned_transaction_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_cancel_unsigned_transaction_operation, (fee)(wallfacer_address)(wallfacer_id)(cancel_trx_id))
