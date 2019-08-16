#pragma once
#include <simplechain/config.h>

namespace simplechain {
	namespace helper {
		bool is_valid_address(const std::string& addr);
		bool is_valid_contract_address(const std::string& addr);
	}
}
