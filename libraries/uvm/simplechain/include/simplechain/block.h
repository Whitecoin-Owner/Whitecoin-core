#pragma once
#include <simplechain/transaction.h>
#include <vector>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

namespace simplechain {
	struct block {
		uint64_t block_number;
		std::string prev_block_hash;
		fc::time_point_sec block_time;
		std::vector<transaction> txs;

		hash_t digest() const;
		std::string digest_str() const;

		fc::mutable_variant_object to_json() const;

		std::string block_hash() const;
	};
}

FC_REFLECT(simplechain::block, (block_number)(prev_block_hash)(block_time)(txs))
