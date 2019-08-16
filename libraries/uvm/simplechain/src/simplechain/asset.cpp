#include <simplechain/asset.h>

namespace simplechain {

}

namespace fc {
	void to_variant(const simplechain::asset& var, variant& vo) {
		fc::mutable_variant_object obj("asset_id", var.asset_id);
		obj("symbol", var.symbol);
		obj("precision", var.precision);
		vo = std::move(obj);
	}
	void from_variant(const variant& var, simplechain::asset& vo) {
		auto obj = var.get_object();
		from_variant(obj["asset_id"], vo.asset_id);
		from_variant(obj["symbol"], vo.symbol);
		from_variant(obj["precision"], vo.precision);
	}
}
