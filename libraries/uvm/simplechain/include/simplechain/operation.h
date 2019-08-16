#pragma once
#include <simplechain/config.h>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

namespace simplechain {

	enum operation_type_enum : int32_t {
		TRANSFER = 0,
		REGISTER_ACCOUNT = 1,

		MINT = 10,

		CONTRACT_CREATE = 101,
		CONTRACT_INVOKE = 102
	};

	struct generic_operation {
		virtual ~generic_operation() {};

		virtual operation_type_enum get_type() const = 0;
		virtual fc::mutable_variant_object to_json() const = 0;
	};
}

FC_REFLECT_ENUM(simplechain::operation_type_enum, (TRANSFER)(REGISTER_ACCOUNT)(CONTRACT_CREATE)(CONTRACT_INVOKE))
