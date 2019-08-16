#include <simplechain/block.h>

namespace simplechain {
	hash_t block::digest() const {
		return hash_t::hash(*this);
	}
	std::string block::digest_str() const {
		return digest().str();
	}
	std::string block::block_hash() const {
		return digest_str();
	}

	fc::mutable_variant_object block::to_json() const {
		fc::mutable_variant_object info;
		info["hash"] = block_hash();
		info["id"] = block_hash();
		info["block_number"] = block_number;
		info["prev_block_hash"] = prev_block_hash;
		info["block_time"] = block_time;
		fc::variants txs_json;
		for (const auto& tx : txs) {
			txs_json.push_back(tx.to_json());
		}
		info["txs"] = txs_json;
		return info;
	}
}
