#pragma once
#include <simplechain/config.h>

namespace simplechain {
	typedef uint32_t asset_id_t;
	typedef uint64_t share_type;
	struct asset {
		asset_id_t asset_id;
		std::string symbol;
		uint32_t precision;
	};
}

namespace fc {
	void to_variant(const simplechain::asset& var, variant& vo);
	void from_variant(const variant& var, simplechain::asset& vo);
}

FC_REFLECT(simplechain::asset, (asset_id)(symbol)(precision))
