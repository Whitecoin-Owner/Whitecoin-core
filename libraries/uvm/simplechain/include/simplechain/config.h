#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <string.h>
#include <vector>
#include <algorithm>
#include <exception>
#include <fc/reflect/reflect.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/filesystem.hpp>
#include <uvm/exceptions.h>

#define SIMPLECHAIN_CORE_ASSET_SYMBOL "COIN"
#define SIMPLECHAIN_CORE_ASSET_PRECISION 5

#define SIMPLECHAIN_CONTRACT_ADDRESS_PREFIX "CON"
#define SIMPLECHAIN_ADDRESS_PREFIX "SPL"
