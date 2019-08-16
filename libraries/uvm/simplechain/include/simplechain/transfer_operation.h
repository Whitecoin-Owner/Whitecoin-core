#pragma once
#include <simplechain/operation.h>
#include <simplechain/evaluate_result.h>
#include <simplechain/asset.h>

namespace simplechain {

	struct register_account_operation : public generic_operation {
		typedef void_evaluate_result result_type;

		operation_type_enum type;
		std::string account_address;
		std::string pubkey_hex;
		fc::time_point_sec op_time;

		register_account_operation() : type(operation_type_enum::REGISTER_ACCOUNT) {}
		virtual ~register_account_operation() {}

		virtual operation_type_enum get_type() const {
			return type;
		}

		virtual fc::mutable_variant_object to_json() const {
			fc::mutable_variant_object info;
			info["type"] = type;
			info["account_address"] = account_address;
			info["pubkey"] = pubkey_hex;
			return info;
		}
	};

	struct mint_operation : public generic_operation {
		typedef void_evaluate_result result_type;

		operation_type_enum type;
		std::string account_address;
		asset_id_t asset_id;
		share_type amount;
		fc::time_point_sec op_time;

		mint_operation() : type(operation_type_enum::MINT) {}
		virtual ~mint_operation() {}

		virtual operation_type_enum get_type() const {
			return type;
		}

		virtual fc::mutable_variant_object to_json() const {
			fc::mutable_variant_object info;
			info["type"] = type;
			info["account_address"] = account_address;
			info["asset_id"] = asset_id;
			info["amount"] = amount;
			info["op_time"] = op_time;
			return info;
		}
	};

	struct transfer_operation : public generic_operation {
		typedef void_evaluate_result result_type;

		operation_type_enum type;
		std::string from_address;
		std::string to_address;
		asset_id_t asset_id;
		share_type amount;
		fc::time_point_sec op_time;

		transfer_operation() : type(operation_type_enum::TRANSFER) {}
		virtual ~transfer_operation() {}

		virtual operation_type_enum get_type() const {
			return type;
		}
		virtual fc::mutable_variant_object to_json() const {
			fc::mutable_variant_object info;
			return info;
		}
	};
}

FC_REFLECT(simplechain::register_account_operation, (type)(account_address)(pubkey_hex)(op_time))
FC_REFLECT(simplechain::mint_operation, (type)(account_address)(asset_id)(amount)(op_time))
FC_REFLECT(simplechain::transfer_operation, (type)(from_address)(to_address)(asset_id)(amount)(op_time))
