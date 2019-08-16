#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene {
	namespace chain {
		class wallfacer_lock_balance_object;
		class wallfacer_lock_balance_object : public graphene::db::abstract_object<wallfacer_lock_balance_object> {
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = wallfacer_lock_balance_object_type;
			wallfacer_member_id_type lock_balance_account;
			asset_id_type lock_asset_id;
			share_type lock_asset_amount;
			wallfacer_lock_balance_object() {};
			asset get_wallfacer_lock_balance() const {
				return asset(lock_asset_amount, lock_asset_id);
			}
			
		};
		struct by_wallfacer_lock;
		using wallfacer_lock_balance_multi_index_type =  multi_index_container <
			wallfacer_lock_balance_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_unique<
			tag<by_wallfacer_lock>,
			composite_key<
			wallfacer_lock_balance_object,
			member<wallfacer_lock_balance_object, wallfacer_member_id_type, &wallfacer_lock_balance_object::lock_balance_account>,
			member<wallfacer_lock_balance_object, asset_id_type, &wallfacer_lock_balance_object::lock_asset_id>
			>,
			composite_key_compare<
			std::less< wallfacer_member_id_type >,
			std::less< asset_id_type >
			>
			>
			>
		> ;
		using  wallfacer_lock_balance_index =  generic_index<wallfacer_lock_balance_object, wallfacer_lock_balance_multi_index_type> ;
	}
}

FC_REFLECT_DERIVED(graphene::chain::wallfacer_lock_balance_object, (graphene::db::object),
					(lock_balance_account)
					(lock_asset_id)
					(lock_asset_amount))
