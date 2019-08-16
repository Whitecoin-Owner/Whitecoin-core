#pragma once
#include <simplechain/config.h>
#include <simplechain/operation.h>
#include <simplechain/contract.h>
#include <simplechain/transfer_operation.h>
#include <fc/variant_object.hpp>
#include <fc/variant.hpp>
#include <fc/static_variant.hpp>

namespace simplechain {
	typedef fc::static_variant<
		register_account_operation,
		mint_operation,
		transfer_operation,
		contract_create_operation,
		contract_invoke_operation,
		native_contract_create_operation
	> operation;

	struct op_wrapper
	{
	public:
		op_wrapper(const operation& op = operation()) :op(op) {}
		operation op;
	};
}

FC_REFLECT_TYPENAME(simplechain::operation)
FC_REFLECT(simplechain::op_wrapper, (op))
