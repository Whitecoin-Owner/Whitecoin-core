#pragma once
#include <cstdint>
#include <string>

struct SimpleUint128 {
    uint64_t big;
    uint64_t low;
};

SimpleUint128 simple_uint128_create(uint64_t big, uint64_t low);

bool simple_uint128_is_zero(const SimpleUint128& a);
// a > b
bool simple_uint128_gt(const SimpleUint128& a, const SimpleUint128& b);
// a < b
bool simple_uint128_lt(const SimpleUint128& a, const SimpleUint128& b);
// a == b
bool simple_uint128_eq(const SimpleUint128& a, const SimpleUint128& b);
// a >= b
bool simple_uint128_ge(const SimpleUint128& a, const SimpleUint128& b);
// a + b
SimpleUint128 simple_uint128_add(const SimpleUint128& a, const SimpleUint128& b);
// a + b
SimpleUint128 simple_uint128_add(const SimpleUint128& a, uint64_t b);
// a - b
SimpleUint128 simple_uint128_minus(const SimpleUint128& a, const SimpleUint128& b);
// - a
SimpleUint128 simple_uint128_neg(const SimpleUint128& a);
// a * b
SimpleUint128 simple_uint128_multi(const SimpleUint128& a, const SimpleUint128& b);
// a * b
SimpleUint128 simple_uint128_multi(const SimpleUint128& a, uint64_t b);

// a ^ p
SimpleUint128 simple_uint128_pow(const SimpleUint128& a, uint8_t p);

// a << b
SimpleUint128 simple_uint128_shift_left(const SimpleUint128& a, uint32_t b);
// a >> b
SimpleUint128 simple_uint128_shift_right(const SimpleUint128& a, uint32_t b);
// a & b
SimpleUint128 simple_uint128_bit_and(const SimpleUint128& a, const SimpleUint128& b);
// a | b
SimpleUint128 simple_uint128_bit_or(const SimpleUint128& a, const SimpleUint128& b);
// ~a
SimpleUint128 simple_uint128_bit_reverse(const SimpleUint128& a);

struct Uint128DivResult {
    SimpleUint128 div_result;
    SimpleUint128 mod_result;
};
// a / b. return div result and mod result. may throw exception
Uint128DivResult simple_uint128_divmod(const SimpleUint128& a, const SimpleUint128& b);
// how many bits does SimpleUint128 have bits who is not 0 from the beginning
uint8_t simple_uint128_bits(const SimpleUint128& a);

std::string simple_uint128_to_string(const SimpleUint128& a, uint8_t base, unsigned int len);

struct SimpleUint256 {
    SimpleUint128 big;
    SimpleUint128 low;
};

SimpleUint256 simple_uint256_create(const SimpleUint128& big, const SimpleUint128& low);

bool simple_uint256_is_zero(const SimpleUint256& a);
// a > b
bool simple_uint256_gt(const SimpleUint256& a, const SimpleUint256& b);
// a == b
bool simple_uint256_eq(const SimpleUint256& a, const SimpleUint256& b);
// a >= b
bool simple_uint256_ge(const SimpleUint256& a, const SimpleUint256& b);
// a + b
SimpleUint256 simple_uint256_add(const SimpleUint256& a, const SimpleUint256& b);
// a - b
SimpleUint256 simple_uint256_minus(const SimpleUint256& a, const SimpleUint256& b);
// - a
SimpleUint256 simple_uint256_neg(const SimpleUint256& a);
// a * b
SimpleUint256 simple_uint256_multi(const SimpleUint256& a, const SimpleUint256& b);
// a << b
SimpleUint256 simple_uint256_shift_left(const SimpleUint256& a, uint32_t b);
// a >> b
SimpleUint256 simple_uint256_shift_right(const SimpleUint256& a, uint32_t b);
// a & b
SimpleUint256 simple_uint256_bit_and(const SimpleUint256& a, const SimpleUint256& b);

struct Uint256DivResult {
    SimpleUint256 div_result;
    SimpleUint256 mod_result;
};
// a / b. return div result and mod result. may throw exception
Uint256DivResult simple_uint256_divmod(const SimpleUint256& a, const SimpleUint256& b);
// how many bits does SimpleUint128 have bits who is not 0 from the beginning
uint8_t simple_uint256_bits(const SimpleUint256& a);

std::string simple_uint256_to_string(const SimpleUint256& a, uint8_t base, unsigned int len);

struct SafeNumber {
    // sign(+/-) x * (10^-e)
    bool valid; // isNaN
    bool sign; // sign == true means positive
    SimpleUint128 x; // in range [0, 10^32] when e > 0
    uint32_t e; // in range [0, 16]. when e > 16 and x in [0, 10^32], this SafeNumber value will be zero
};

SafeNumber safe_number_zero();

// create NaN number
SafeNumber safe_number_create_invalid();

bool safe_number_is_valid(const SafeNumber& a);

bool safe_number_invalid(const SafeNumber& a);

SafeNumber safe_number_create(bool sign, const SimpleUint128& x, uint32_t e);

SafeNumber safe_number_create(bool sign, const uint64_t& x, uint32_t e);

SafeNumber safe_number_create(int64_t value);

SafeNumber safe_number_create(const SimpleUint128& value);

SafeNumber safe_number_create(const std::string& str);

// a + b
SafeNumber safe_number_add(const SafeNumber& a, const SafeNumber& b);

// a == 0
bool safe_number_is_zero(const SafeNumber& a);

// - a
SafeNumber safe_number_neg(const SafeNumber& a);

// a - b
SafeNumber safe_number_minus(const SafeNumber& a, const SafeNumber& b);

// a * b
SafeNumber safe_number_multiply(const SafeNumber& a, const SafeNumber& b);

// a / b
SafeNumber safe_number_div(const SafeNumber& a, const SafeNumber& b);

// toint(a/b)
SafeNumber safe_number_idiv(const SafeNumber& a, const SafeNumber& b);

// a % b
SafeNumber safe_number_mod(const SafeNumber& a, const SafeNumber& b);

// abs(a)
SafeNumber safe_number_abs(const SafeNumber& a);

// tostring(a)
std::string safe_number_to_string(const SafeNumber& a);

int64_t safe_number_to_int64(const SafeNumber& a); // 12.3 -> 12    -12.3 -> -12

int64_t safe_number_to_int64_floor(const SafeNumber& a); //// 12.3 -> 12    -12.3 -> -13

// a == b. a and b should both be compressed
bool safe_number_eq(const SafeNumber& a, const SafeNumber& b);

// a != b
bool safe_number_ne(const SafeNumber& a, const SafeNumber& b);

// a > b
bool safe_number_gt(const SafeNumber& a, const SafeNumber& b);

// a >= b
bool safe_number_gte(const SafeNumber& a, const SafeNumber& b);

// a < b
bool safe_number_lt(const SafeNumber& a, const SafeNumber& b);

// a <= b
bool safe_number_lte(const SafeNumber& a, const SafeNumber& b);


namespace std {
    std::string to_string(const SimpleUint128& value);
    std::string to_string(const SimpleUint256& value);
    std::string to_string(const SafeNumber& value);
}
