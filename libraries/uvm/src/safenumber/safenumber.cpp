#include "safenumber/safenumber.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <cstdio>
#include <cmath>

const std::string NaN_str = "NaN";

static uint64_t uint64_pow(uint64_t a, int p) {
    if(a==10) {
        switch (p) {
            case 0: return 1;
            case 1: return 10;
            case 2: return 100;
            case 3: return 1000;
            case 4: return 10000;
            case 5: return 100000;
            case 6: return 1000000;
            case 7: return 10000000L;
            case 8: return 100000000L;
            case 9: return 1000000000L;
            case 16: return 10000000000000000L;
            default: {}
        }
    }
	return static_cast<uint64_t >(std::pow(static_cast<uint64_t>(a), p));
}


SimpleUint128 simple_uint128_create(uint64_t big, uint64_t low) {
	SimpleUint128 result {};
	result.big = big;
	result.low = low;
	return result;
}

bool simple_uint128_is_zero(const SimpleUint128& a) {
    return a.big == 0 && a.low == 0;
}

const SimpleUint128 uint128_0 = simple_uint128_create(0, 0);
const SimpleUint128 uint128_1 = simple_uint128_create(0, 1);
const SimpleUint128 uint128_10 = simple_uint128_create(0, 10);
const uint64_t uint64_bigest = 0xffffffffffffffffL;
const SimpleUint128 uint128_bigest = simple_uint128_create(uint64_bigest, uint64_bigest);

bool simple_uint128_gt(const SimpleUint128& a, const SimpleUint128& b) {
	if (a.big == b.big){
		return (a.low > b.low);
	}
	return (a.big > b.big);
}

bool simple_uint128_lt(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_gt(b, a);
}

bool simple_uint128_eq(const SimpleUint128& a, const SimpleUint128& b) {
	return ((a.big == b.big) && (a.low == b.low));
}

bool simple_uint128_ge(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_gt(a, b) || simple_uint128_eq(a, b);
}

SimpleUint128 simple_uint128_add(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_create(a.big + b.big + ((a.low + b.low) < a.low ? 1 : 0), a.low + b.low);
}

SimpleUint128 simple_uint128_add(const SimpleUint128& a, uint64_t b) {
	return simple_uint128_add(a, simple_uint128_create(0, b));
}

SimpleUint128 simple_uint128_minus(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_create(a.big - b.big - ((a.low - b.low) > a.low ? 1 : 0), a.low - b.low);
}

SimpleUint128 simple_uint128_neg(const SimpleUint128& a) {
	SimpleUint128 reverse_a;
	reverse_a.big = ~a.big;
	reverse_a.low = ~a.low;
	return simple_uint128_add(reverse_a, uint128_1);
}

SimpleUint128 simple_uint128_multi(const SimpleUint128& a, const SimpleUint128& b) {
	// split values into 4 32-bit parts
	uint64_t top[4] = {a.big >> 32, a.big & 0xffffffff, a.low >> 32, a.low & 0xffffffff};
	uint64_t bottom[4] = {b.big >> 32, b.big & 0xffffffff, b.low >> 32, b.low & 0xffffffff};
	uint64_t products[4][4];

	// multiply each component of the values
	for(int y = 3; y > -1; y--){
		for(int x = 3; x > -1; x--){
			products[3 - x][y] = top[x] * bottom[y];
		}
	}

	// first row
	uint64_t fourth32 = (products[0][3] & 0xffffffff);
	uint64_t third32  = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
	uint64_t second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
	uint64_t first32  = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);

	// second row
	third32  += (products[1][3] & 0xffffffff);
	second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
	first32  += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);

	// third row
	second32 += (products[2][3] & 0xffffffff);
	first32  += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);

	// fourth row
	first32  += (products[3][3] & 0xffffffff);

	// move carry to next digit
	third32  += fourth32 >> 32;
	second32 += third32  >> 32;
	first32  += second32 >> 32;

	// remove carry from current digit
	fourth32 &= 0xffffffff;
	third32  &= 0xffffffff;
	second32 &= 0xffffffff;
	first32  &= 0xffffffff;

	// combine components
	auto result_big = (first32 << 32) | second32;
	auto result_low = (third32 << 32) | fourth32;
	return simple_uint128_create(result_big, result_low);
}

SimpleUint128 simple_uint128_multi(const SimpleUint128& a, uint64_t b) {
	return simple_uint128_multi(a, simple_uint128_create(0, b));
}

SimpleUint128 simple_uint128_pow(const SimpleUint128& a, uint8_t p) {
	SimpleUint128 result = uint128_1;
	// TODO: change to multiply only log(p) times
	for(uint8_t i = 0;i<p;i++) {
		result = simple_uint128_multi(result, a);
	}
	return result;
}

SimpleUint128 simple_uint128_shift_left(const SimpleUint128& a, uint32_t b) {
	uint32_t shift = b;
	if (shift >= 128){
		return uint128_0;
	}
	else if (shift == 64){
		return simple_uint128_create(a.low, 0);
	}
	else if (shift == 0){
		return a;
	}
	else if (shift < 64){
		return simple_uint128_create((a.big << shift) + (a.low >> (64 - shift)), a.low << shift);
	}
	else if ((128 > shift) && (shift > 64)){
		return simple_uint128_create(a.low << (shift - 64), 0);
	}
	else{
		return uint128_0;
	}
}

SimpleUint128 simple_uint128_shift_right(const SimpleUint128& a, uint32_t b) {
	const uint32_t shift = b;
	if (shift >= 128){
		return uint128_0;
	}
	else if (shift == 64){
		return simple_uint128_create(0, a.big);
	}
	else if (shift == 0){
		return a;
	}
	else if (shift < 64){
		return simple_uint128_create(a.big >> shift, (a.big << (64 - shift)) + (a.low >> shift));
	}
	else if ((128 > shift) && (shift > 64)){
		return simple_uint128_create(0, (a.big >> (shift - 64)));
	}
	else{
		return uint128_0;
	}
}

SimpleUint128 simple_uint128_bit_and(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_create(a.big & b.big, a.low & b.low);
}

SimpleUint128 simple_uint128_bit_or(const SimpleUint128& a, const SimpleUint128& b) {
	return simple_uint128_create(a.big | b.big, a.low | b.low);
}

SimpleUint128 simple_uint128_bit_reverse(const SimpleUint128& a) {
	SimpleUint128 result = {};
	result.big = ~a.big;
	result.low = ~a.low;
	return result;
}

static Uint128DivResult make_uint128_div_result(const SimpleUint128& div_result, const SimpleUint128& mod_result) {
	Uint128DivResult result{};
	result.div_result = div_result;
	result.mod_result = mod_result;
	return result;
}

Uint128DivResult simple_uint128_divmod(const SimpleUint128& a, const SimpleUint128& b) {
	if (b.big == 0 && b.low == 0){
		throw std::domain_error("Error: division or modulus by 0");
	}
	else if (b.big == 0 && b.low == 1){
		return make_uint128_div_result(a, uint128_0);
	}
	else if (a.big == b.big && a.low == b.low){
		return make_uint128_div_result(uint128_1, uint128_0);
	}
	else if ((a.big == 0 && a.low == 0) || (a.big < b.big || (a.big==b.big && a.low<b.low))){
		return make_uint128_div_result(uint128_0, a);
	}
	SimpleUint128 div_result = simple_uint128_create(0, 0);
	SimpleUint128 mod_result = simple_uint128_create(0, 0);
	for(uint8_t x = simple_uint128_bits(a); x > 0; x--){
		div_result = simple_uint128_shift_left(div_result, 1);
		mod_result = simple_uint128_shift_left(mod_result, 1);

		auto tmp1 = simple_uint128_shift_right(a, x-1U);
		auto tmp2 = simple_uint128_bit_and(tmp1, uint128_1);
		if (tmp2.big || tmp2.low){
			mod_result = simple_uint128_add(mod_result, uint128_1);
		}
		if (simple_uint128_ge(mod_result, b)){
			mod_result = simple_uint128_minus(mod_result, b);
			div_result = simple_uint128_add(div_result, uint128_1);
		}
	}
	return make_uint128_div_result(div_result, mod_result);
}

uint8_t simple_uint128_bits(const SimpleUint128& a) {
	uint8_t out = 0;
	if (a.big){
		out = 64;
		uint64_t up = a.big;
		while (up){
			up >>= 1;
			out++;
		}
	}
	else{
		uint64_t low = a.low;
		while (low){
			low >>= 1;
			out++;
		}
	}
	return out;
}

std::string simple_uint128_to_string(const SimpleUint128& a, uint8_t base, unsigned int len) {
	if ((base < 2) || (base > 16)){
		throw std::invalid_argument("Base must be in the range [2, 16]");
	}
	std::string out = "";
	if (simple_uint128_is_zero(a)){
		out = "0";
	}
	else{
		auto qr = make_uint128_div_result(a, uint128_0);
		auto base_uint128 = simple_uint128_create(0, base);
		do{
			qr = simple_uint128_divmod(qr.div_result, base_uint128);
			out = "0123456789abcdef"[(uint8_t) qr.mod_result.low] + out;
		} while (!simple_uint128_is_zero(qr.div_result));
	}
	if (out.size() < len){
		out = std::string(len - out.size(), '0') + out;
	}
	return out;
}

// SimpleUint256 start
SimpleUint256 simple_uint256_create(const SimpleUint128& big, const SimpleUint128& low) {
    SimpleUint256 result;
    result.big = big;
    result.low = low;
    return result;
}

bool simple_uint256_is_zero(const SimpleUint256& a) {
	return simple_uint128_is_zero(a.big) && simple_uint128_is_zero(a.low);
}

const SimpleUint256 uint256_0 = simple_uint256_create(uint128_0, uint128_0);
const SimpleUint256 uint256_1 = simple_uint256_create(uint128_0, uint128_1);
const SimpleUint256 uint256_10 = simple_uint256_create(uint128_0, uint128_10);

bool simple_uint256_gt(const SimpleUint256& a, const SimpleUint256& b) {
	if (simple_uint128_eq(a.big, b.big)){
		return simple_uint128_gt(a.low, b.low);
	}
	return simple_uint128_gt(a.big, b.big);
}

bool simple_uint256_eq(const SimpleUint256& a, const SimpleUint256& b) {
	return (simple_uint128_eq(a.big, b.big) && simple_uint128_eq(a.low, b.low));
}

bool simple_uint256_ge(const SimpleUint256& a, const SimpleUint256& b) {
	return simple_uint256_gt(a, b) || simple_uint256_eq(a, b);
}

SimpleUint256 simple_uint256_add(const SimpleUint256& a, const SimpleUint256& b) {
	const auto& abig_plus_bbig = simple_uint128_add(a.big, b.big);
	const auto& alow_plus_blow = simple_uint128_add(a.low, b.low);
	return simple_uint256_create(simple_uint128_add(abig_plus_bbig, simple_uint128_lt(alow_plus_blow, a.low) ? 1 : 0), alow_plus_blow);
}

SimpleUint256 simple_uint256_minus(const SimpleUint256& a, const SimpleUint256& b) {
	const auto& abig_minus_bbig = simple_uint128_minus(a.big, b.big);
	const auto& alow_minus_blow = simple_uint128_minus(a.low, b.low);
	return simple_uint256_create(simple_uint128_add(abig_minus_bbig, simple_uint128_gt(alow_minus_blow, a.low) ? -1 : 0), alow_minus_blow);
}

SimpleUint256 simple_uint256_neg(const SimpleUint256& a) {
	SimpleUint256 reverse_a;
	reverse_a.big = simple_uint128_bit_reverse(a.big);
	reverse_a.low = simple_uint128_bit_reverse(a.low);
	return simple_uint256_add(reverse_a, uint256_1);
}

SimpleUint256 simple_uint256_multi(const SimpleUint256& a, const SimpleUint256& b) {
	// split values into 4 64-bit parts
	SimpleUint128 mask = simple_uint128_create(0, uint64_bigest);
	SimpleUint128 top[4] = {simple_uint128_shift_right(a.big, 64), simple_uint128_bit_and(a.big, mask), simple_uint128_shift_right(a.low, 64), simple_uint128_bit_and(a.low, mask) };
	SimpleUint128 bottom[4] = { simple_uint128_shift_right(b.big, 64), simple_uint128_bit_and(b.big, mask), simple_uint128_shift_right(b.low, 64), simple_uint128_bit_and(b.low, mask) };
	SimpleUint128 products[4][4];

	// multiply each component of the values
	for(int y = 3; y > -1; y--){
		for(int x = 3; x > -1; x--){
			products[3 - x][y] = simple_uint128_multi(top[x], bottom[y]);
		}
	}

	// first row
	SimpleUint128 fourth32 = simple_uint128_bit_and(products[0][3], mask);
	SimpleUint128 third32  = simple_uint128_add(simple_uint128_bit_and(products[0][2], mask), simple_uint128_shift_right(products[0][3], 64));
	SimpleUint128 second32 = simple_uint128_add(simple_uint128_bit_and(products[0][1], mask), simple_uint128_shift_right(products[0][2], 64));
	SimpleUint128 first32  = simple_uint128_add(simple_uint128_bit_and(products[0][0], mask), simple_uint128_shift_right(products[0][1], 64));

	// second row
	third32  = simple_uint128_add(third32, simple_uint128_bit_and(products[1][3], mask));
	second32 = simple_uint128_add(second32, simple_uint128_add(simple_uint128_bit_and(products[1][2], mask), simple_uint128_shift_right(products[1][3], 64)));
	first32  = simple_uint128_add(first32, simple_uint128_add(simple_uint128_bit_and(products[1][1], mask), simple_uint128_shift_right(products[1][2], 64)));

	// third row
	second32 = simple_uint128_add(second32, simple_uint128_bit_and(products[2][3], mask));
	first32  = simple_uint128_add(first32, simple_uint128_add(simple_uint128_bit_and(products[2][2], mask), simple_uint128_shift_right(products[2][3], 64)));

	// fourth row
	first32  = simple_uint128_add(first32, simple_uint128_bit_and(products[3][3], mask));

	// move carry to next digit
	third32  = simple_uint128_add(third32, simple_uint128_shift_right(fourth32, 64));
	second32 = simple_uint128_add(second32, simple_uint128_shift_right(third32, 64));
	first32  = simple_uint128_add(first32, simple_uint128_shift_right(second32, 64));

	// remove carry from current digit
	fourth32 = simple_uint128_bit_and(fourth32, mask);
	third32  = simple_uint128_bit_and(third32, mask);
	second32 = simple_uint128_bit_and(second32, mask);
	first32  = simple_uint128_bit_and(first32, mask);

	// combine components
	const auto& result_big = simple_uint128_bit_or(simple_uint128_shift_left(first32, 64), second32);
	const auto& result_low = simple_uint128_bit_or(simple_uint128_shift_left(third32, 64), fourth32);
	return simple_uint256_create(result_big, result_low);
}

SimpleUint256 simple_uint256_shift_left(const SimpleUint256& a, uint32_t b) {
	uint32_t shift = b;
	if (shift >= 256){
		return uint256_0;
	}
	else if (shift == 128){
		return simple_uint256_create(a.low, uint128_0);
	}
	else if (shift == 0){
		return a;
	}
	else if (shift < 128){
		return simple_uint256_create(simple_uint128_add(simple_uint128_shift_left(a.big, shift), simple_uint128_shift_right(a.low, 128 - shift)), simple_uint128_shift_left(a.low, shift));
	}
	else if ((256 > shift) && (shift > 128)){
		return simple_uint256_create(simple_uint128_shift_left(a.low, shift - 128), uint128_0);
	}
	else{
		return uint256_0;
	}
}

SimpleUint256 simple_uint256_shift_right(const SimpleUint256& a, uint32_t b) {
	const uint32_t shift = b;
	if (shift >= 256){
		return uint256_0;
	}
	else if (shift == 128){
		return simple_uint256_create(uint128_0, a.big);
	}
	else if (shift == 0){
		return a;
	}
	else if (shift < 128){
		return simple_uint256_create(simple_uint128_shift_right(a.big, shift), simple_uint128_add(simple_uint128_shift_left(a.big, 128 - shift), simple_uint128_shift_right(a.low, shift)));
	}
	else if ((256 > shift) && (shift > 128)){
		return simple_uint256_create(uint128_0, simple_uint128_shift_right(a.big, shift - 128));
	}
	else{
		return uint256_0;
	}
}

SimpleUint256 simple_uint256_bit_and(const SimpleUint256& a, const SimpleUint256& b) {
	return simple_uint256_create(simple_uint128_bit_and(a.big, b.big), simple_uint128_bit_and(a.low, b.low));
}

SimpleUint256 simple_uint256_bit_reverse(const SimpleUint256& a) {
	SimpleUint256 result = {};
	result.big = simple_uint128_bit_reverse(a.big);
	result.low = simple_uint128_bit_reverse(a.low);
	return result;
}

static Uint256DivResult make_uint256_div_result(const SimpleUint256& div_result, const SimpleUint256& mod_result) {
	Uint256DivResult result{};
	result.div_result = div_result;
	result.mod_result = mod_result;
	return result;
}

Uint256DivResult simple_uint256_divmod(const SimpleUint256& a, const SimpleUint256& b) {
	if (simple_uint256_is_zero(b)) {
		throw std::domain_error("Error: division or modulus by 0");
	}
	else if (simple_uint128_is_zero(b.big) && simple_uint128_eq(b.low, uint128_1)) {
		return make_uint256_div_result(a, uint256_0);
	}
	else if (simple_uint128_eq(a.big, b.big) && simple_uint128_eq(a.low, b.low)){
		return make_uint256_div_result(uint256_1, uint256_0);
	}
	else if ((simple_uint128_is_zero(a.big) && simple_uint128_is_zero(a.low)) || (simple_uint128_lt(a.big, b.big) || (simple_uint128_eq(a.big, b.big) && simple_uint128_lt(a.low, b.low)))){
		return make_uint256_div_result(uint256_0, a);
	}
	SimpleUint256 div_result = uint256_0;
	SimpleUint256 mod_result = uint256_0;
	for(uint8_t x = simple_uint256_bits(a); x > 0; x--){
		div_result = simple_uint256_shift_left(div_result, 1);
		mod_result = simple_uint256_shift_left(mod_result, 1);

		auto tmp1 = simple_uint256_shift_right(a, x - 1U);
		auto tmp2 = simple_uint256_bit_and(tmp1, uint256_1);
		if (!simple_uint256_is_zero(tmp2)){
			mod_result = simple_uint256_add(mod_result, uint256_1);
		}
		if (simple_uint256_ge(mod_result, b)){
			mod_result = simple_uint256_minus(mod_result, b);
			div_result = simple_uint256_add(div_result, uint256_1);
		}
	}
	return make_uint256_div_result(div_result, mod_result);
}

uint8_t simple_uint256_bits(const SimpleUint256& a) {
    uint8_t out = 0;
    if (!simple_uint128_is_zero(a.big)){
        out = 128;
        SimpleUint128 up = a.big;
        while (!simple_uint128_is_zero(up)){
            up = simple_uint128_shift_right(up, 1);
            out++;
        }
    }
    else{
        SimpleUint128 low = a.low;
        while (!simple_uint128_is_zero(low)){
            low = simple_uint128_shift_right(low, 1);
            out++;
        }
    }
    return out;
}

std::string simple_uint256_to_string(const SimpleUint256& a, uint8_t base, unsigned int len) {
	if ((base < 2) || (base > 16)){
		throw std::invalid_argument("Base must be in the range [2, 16]");
	}
	std::string out = "";
	if (simple_uint256_is_zero(a)){
		out = "0";
	}
	else{
		auto qr = make_uint256_div_result(a, uint256_0);
		auto base_uint256 = simple_uint256_create(uint128_0, simple_uint128_create(0, base));
		do{
			qr = simple_uint256_divmod(qr.div_result, base_uint256);
			out = "0123456789abcdef"[(uint8_t) qr.mod_result.low.low] + out;
		} while (!simple_uint256_is_zero(qr.div_result));
	}
	if (out.size() < len){
		out = std::string(len - out.size(), '0') + out;
	}
	return out;
}

// SimpleUint256 end


SafeNumber safe_number_zero() {
	SafeNumber n;
	n.valid = true;
	n.sign = true;
	n.x = uint128_0;
	n.e = 0;
	return n;
}


SafeNumber safe_number_create_invalid() {
	SafeNumber n;
	n.valid = false;
	n.sign = true;
	n.x = uint128_0;
	n.e = 0;
	return n;
}

const SimpleUint128 largest_x = simple_uint128_multi(simple_uint128_create(0, uint64_pow(10, 16)), simple_uint128_create(0, uint64_pow(10, 16)));

bool safe_number_is_valid(const SafeNumber& a) {
	return a.valid;
}

bool safe_number_invalid(const SafeNumber& a) {
    return !a.valid;
}

static SafeNumber compress_number(const SafeNumber& a) {
	// compress a's value. If a.x is a multiple of 10, the value of a.e can be reduced accordingly.
	if(!safe_number_is_valid(a)) {
		return a;
	}
	SimpleUint128 x = a.x;
	uint32_t e = a.e;
	// when a.x * b.x is too big. should short result x and result e. Requires certainty and does not require complete accuracy
	while(simple_uint128_gt(x, largest_x)) {
		if(e == 0)
			break;
		x = simple_uint128_divmod(x, uint128_10).div_result; // x / 10
		--e;
	}
	uint32_t tmp_x_large_count = e;
	while(tmp_x_large_count > 16) { // eg. when a = 1.1234567890123
		x = simple_uint128_divmod(x, uint128_10).div_result; // x / 10
		tmp_x_large_count--;
	}
	while(tmp_x_large_count < e) {
		x = simple_uint128_multi(x, uint128_10); // x * 10;
		++tmp_x_large_count;
	}
	while(!simple_uint128_is_zero(x) && simple_uint128_eq(simple_uint128_multi(simple_uint128_divmod(x, uint128_10).div_result, uint128_10), x) && e > 0) {
		x = simple_uint128_divmod(x, uint128_10).div_result; // x / 10
		e--;
	}
	auto sign = a.sign;
	if(e > 16) {
		x = uint128_0;
		e = 0;
		sign = true;
	}
	SafeNumber n;
	n.valid = true;
	n.sign = sign;
	n.x = x;
	n.e = e;
	return n;
}

SafeNumber safe_number_create(bool sign, const SimpleUint128& x, uint32_t e) {
	SafeNumber n;
	n.valid = true;
	n.sign = sign;
	n.x = x;
	n.e = e;
	return compress_number(n);
}

SafeNumber safe_number_create(bool sign, const uint64_t& x, uint32_t e) {
	return safe_number_create(sign, simple_uint128_create(0, x), e);
}

SafeNumber safe_number_create(int64_t value) {
	SafeNumber n;
	n.valid = true;
	n.sign = value >= 0;
	auto value_uint = static_cast<uint64_t>(value >= 0 ? value : -value);
	n.x = simple_uint128_create(0, value_uint);
	n.e = 0;
	return compress_number(n);
}

SafeNumber safe_number_create(const SimpleUint128& value) {
	SafeNumber n;
	n.valid = true;
	n.sign = true;
	n.x = value;
	n.e = 0;
	return compress_number(n);
}

const SafeNumber sn_0 = safe_number_zero();
const SafeNumber sn_1 = safe_number_create(true, uint128_1, 0);
const SafeNumber sn_nan = safe_number_create_invalid();

SafeNumber safe_number_create(const std::string& str) {
	// xxxxx.xxxxx to SafeNumber
	auto str_size = str.size();
	auto str_chars = str.c_str();
	if (0 == str_size) {
		return sn_nan;
	}
	if (str_size > 40) {
		return sn_nan;
	}
	if (str == NaN_str) {
		return sn_nan;
	}
	auto has_sign_char = str_chars[0] == '+' || str_chars[0] == '-';
	auto result_sign = str_chars[0] != '-';
	size_t pos = has_sign_char ? 1 : 0;
	bool after_dot = false;
	// sign * p.q & 10^-e
	SimpleUint128 x = uint128_0;
	uint32_t e = 0;
	while (pos < str_size) {
		auto c = str_chars[pos];
		auto c_int = c - '0';
		if (c == '.') {
			if (after_dot) {
				return sn_nan;
			}
			pos++;
			after_dot = true;
			continue;
		}
		x = simple_uint128_add(simple_uint128_multi(x, uint128_10), c_int); // x = 10 * x + c_int;
		if (after_dot) {
			e++;
		}
		pos++;
	}
	auto result_x = x;
	auto result_e = e;
	return safe_number_create(result_sign, result_x, result_e);
}

// a + b
SafeNumber safe_number_add(const SafeNumber& a, const SafeNumber& b) {
	if (!safe_number_is_valid(a) || !safe_number_is_valid(b)) {
		return a;
	}
	if (safe_number_is_zero(b))
		return a;
	if (safe_number_is_zero(a))
		return b;
	auto max_e = std::max(a.e, b.e);

	SimpleUint128 ax = simple_uint128_multi(a.x, uint64_pow(10, max_e - a.e)); // a = sign * (x*100) * 10^-(e+2=max_e) if max_e - a.e = 2
	SimpleUint128 bx = simple_uint128_multi(b.x, uint64_pow(10, max_e - b.e));

	SimpleUint128 result_x {};
	bool result_sign;
	if(a.sign != b.sign) {
		if(simple_uint128_gt(ax, bx)) {
			result_sign = a.sign;
			result_x = simple_uint128_minus(ax, bx);
		} else if(simple_uint128_eq(ax, bx)) {
			result_sign = true;
			result_x = uint128_0;
		} else {
			result_sign = b.sign;
			result_x = simple_uint128_minus(bx, ax);
		}
	} else {
		result_sign = a.sign;
		result_x = simple_uint128_add(ax, bx);
	}
	auto result_e = max_e;
	return safe_number_create(result_sign, result_x, result_e);
}

bool safe_number_is_zero(const SafeNumber& a) {
	if (!a.valid)
		return false;
	return simple_uint128_is_zero(a.x);
}

// - a
SafeNumber safe_number_neg(const SafeNumber& a) {
	if (!safe_number_is_valid(a)) {
		return a;
	}
	if (safe_number_is_zero(a))
		return a;
	return safe_number_create(!a.sign, a.x, a.e);
}

// a - b
SafeNumber safe_number_minus(const SafeNumber& a, const SafeNumber& b) {
	return safe_number_add(a, safe_number_neg(b));
}

// a * b
SafeNumber safe_number_multiply(const SafeNumber& a, const SafeNumber& b) {
	if (!safe_number_is_valid(a) || !safe_number_is_valid(b)) {
		return a;
	}
	if (safe_number_is_zero(a) || safe_number_is_zero(b))
		return sn_0;
	bool result_sign = a.sign == b.sign;
	auto result_e = a.e + b.e;
	auto ax = simple_uint256_create(uint128_0, a.x);
	auto bx = simple_uint256_create(uint128_0, b.x);
	auto abx = simple_uint256_multi(ax, bx); // a.x * b.x
	while(!simple_uint128_is_zero(abx.big)) { // when abx is overflow as SimpleUint128 format
		if(result_e==0) {
			break;
		}
		abx = simple_uint256_divmod(abx, uint256_10).div_result; // abx = abx/10
		--result_e;
	}
	auto result_x = abx.low; // throw abx's overflow upper part
	return safe_number_create(result_sign, result_x, result_e);
}

// a / b = a.x / b.x * 10^(b-a)
SafeNumber safe_number_div(const SafeNumber& a, const SafeNumber& b) {
	if (!safe_number_is_valid(a) || !safe_number_is_valid(b)) {
		return a;
	}
	if (safe_number_is_zero(a))
		return sn_0;
	if (safe_number_is_zero(b))
		return sn_nan;
	auto sign = a.sign == b.sign;
	int32_t extra_e = a.e-b.e; // r.e need to add extra_e. if extra_e < -r.e then r.x = r.x * 10^(-extra_e - r.e) and r.e = 0
	SimpleUint256 big_a = simple_uint256_create(uint128_0, a.x);
	SimpleUint256 big_b = simple_uint256_create(uint128_0, b.x);
    // The resulting fractional form is big_a/big_b
	// Approximate the score out of the decimal number
	// Let the result be SafeNumber r, initial value is r.x = 0, r.e = -1
	SimpleUint128 rx = uint128_0; // r.x
	int32_t re = -1; // r.e
	// r.x = r.x* 10 + big_a/big_b, big_a = (big_a % big_b) * 10, r.e += 1, Repeat this step until r.e >= 16 or big_a == 0 or rx > largest_x
	do {
		const auto& divmod = simple_uint256_divmod(big_a, big_b);
		rx = simple_uint128_add(simple_uint128_multi(rx, 10), divmod.div_result.low); // ignore overflowed value
		big_a = simple_uint256_multi(divmod.mod_result, uint256_10);
		++re;
	}while(re<16 && simple_uint128_lt(rx, largest_x) && !simple_uint256_is_zero (big_a));
	if(extra_e>=-static_cast<int32_t>(re)) {
		re += extra_e;
	} else {
		rx = simple_uint128_multi(rx, uint64_pow(10, -extra_e - re));
		re = 0;
	}
	return safe_number_create(sign, rx, static_cast<uint32_t>(re));
}

SafeNumber safe_number_idiv(const SafeNumber& a, const SafeNumber& b) {
	const auto& div_result = safe_number_div(a, b);
	const auto& div_result_int = safe_number_to_int64(div_result);
	if(div_result.sign) {
		return safe_number_create(div_result_int);
	} else {
		return safe_number_create(div_result_int-1);
	}
}

SafeNumber safe_number_mod(const SafeNumber& a, const SafeNumber& b) {
    const auto& div_result = safe_number_idiv(a, b);
    const auto& mod_result = safe_number_minus(a, safe_number_multiply(b, div_result));
    return mod_result;
}

SafeNumber safe_number_abs(const SafeNumber& a) {
    const auto& r = a.sign ? a : safe_number_neg(a);
    return r;
}

// tostring(a)
std::string safe_number_to_string(const SafeNumber& a) {
	const auto& val = compress_number(a);
	if (!safe_number_is_valid(val))
		return NaN_str;
	if (safe_number_is_zero(val))
		return "0";
	std::stringstream ss;
	if (!val.sign) {
		ss << "-";
	}
	// x = p.q
	auto e10 = uint64_pow(10, val.e);
	const auto& e10_uint = simple_uint128_create(0, e10);
	const auto& pq = simple_uint128_divmod(val.x, e10_uint);
	const auto& p = pq.div_result;
	const auto& q = pq.mod_result;
	ss << simple_uint128_to_string(p, 10, 0);

	auto decimal_len = val.e;
	if (!simple_uint128_is_zero(q)) {
		auto decimal = simple_uint128_to_string(q, 10, decimal_len);
		ss << "." << decimal;
	}
	return ss.str();
}

int64_t safe_number_to_int64(const SafeNumber& a) {
	if(safe_number_invalid(a)) {
		return 0;
	}
	if(safe_number_is_zero(a)) {
		return 0;
	}
	int64_t result = 0;
	if(a.e > 0) {
		auto ax_divmod = simple_uint128_divmod(a.x, simple_uint128_create(0, uint64_pow(10, a.e)));
		auto tmp_uint128 = ax_divmod.div_result;
		int64_t tmp = (static_cast<int64_t>(tmp_uint128.low) << 1) >> 1;
		result = a.sign ? tmp : -tmp;
	}
	else {
		int64_t tmp = (static_cast<int64_t>(a.x.low) << 1) >> 1;
		result = a.sign ? tmp : -tmp;
	}
	return result;
}


int64_t safe_number_to_int64_floor(const SafeNumber& a) {
	if (safe_number_invalid(a)) {
		return 0;
	}
	if (safe_number_is_zero(a)) {
		return 0;
	}
	int64_t result = 0;
	if (a.e > 0) {
		auto ax_divmod = simple_uint128_divmod(a.x, simple_uint128_create(0, uint64_pow(10, a.e)));
		auto tmp_uint128 = ax_divmod.div_result;
		auto mod_result = ax_divmod.mod_result;
		int64_t tmp = (static_cast<int64_t>(tmp_uint128.low) << 1) >> 1;

		if (simple_uint128_is_zero(mod_result)) {
			result = a.sign ? tmp : -tmp;
		}
		else {
			result = a.sign ? tmp : (-tmp-1);
		}
	}
	else {
		int64_t tmp = (static_cast<int64_t>(a.x.low) << 1) >> 1;
		result = a.sign ? tmp : -tmp;
	}
	return result;
}

bool safe_number_eq(const SafeNumber& a, const SafeNumber& b) {
	if(safe_number_invalid(a) || safe_number_invalid(b)) {
		return false;
	}
	return a.sign == b.sign && simple_uint128_eq(a.x, b.x) && a.e == b.e;
}

bool safe_number_ne(const SafeNumber& a, const SafeNumber& b) {
	if(safe_number_invalid(a) || safe_number_invalid(b)) {
		return false;
	}
	return !safe_number_eq(a, b);
}

bool safe_number_gt(const SafeNumber& a, const SafeNumber& b) {
	if(safe_number_invalid(a) || safe_number_invalid(b)) {
		return false;
	}
	if(a.sign != b.sign) {
		return a.sign;
	}
	const auto& a_divmod = simple_uint128_divmod(a.x, simple_uint128_create(0, uint64_pow(10, a.e)));
	const auto& b_divmod = simple_uint128_divmod(b.x, simple_uint128_create(0, uint64_pow(10, b.e)));

	if(simple_uint128_gt(a_divmod.div_result, b_divmod.div_result)) {
		return a.sign;
	} else if(simple_uint128_lt(a_divmod.div_result, b_divmod.div_result)) {
		return !a.sign;
	}
	auto extend_a_remaining = a_divmod.mod_result;
	auto extend_b_remaining = b_divmod.mod_result;
	if(a.e > b.e) {
		extend_b_remaining = simple_uint128_multi(extend_b_remaining, uint64_pow(10, a.e - b.e));
	} else if(a.e < b.e) {
		extend_a_remaining = simple_uint128_multi(extend_a_remaining, uint64_pow(10, b.e - a.e));
	}
	if(simple_uint128_eq(extend_a_remaining, extend_b_remaining)) {
		return false;
	}
	if(simple_uint128_gt(extend_a_remaining, extend_b_remaining)) {
		return a.sign;
	}
	// extend_a_remaining < extend_b_remaining
	return !a.sign;
}

// a >= b
bool safe_number_gte(const SafeNumber& a, const SafeNumber& b) {
	return safe_number_eq(a, b) || safe_number_gt(a, b);
}

// a < b
bool safe_number_lt(const SafeNumber& left, const SafeNumber& right) {
	return safe_number_gt(right, left);
}

// a <= b
bool safe_number_lte(const SafeNumber& a, const SafeNumber& b) {
	return safe_number_eq(a, b) || safe_number_lt(a, b);
}

namespace std {
	std::string to_string(const SimpleUint128& value) {
		return simple_uint128_to_string(value, 10, 0);
	}

	std::string to_string(const SimpleUint256& value) {
		return simple_uint256_to_string(value, 10, 0);
	}

	std::string to_string(const SafeNumber& value) {
		return safe_number_to_string(value);
	}
}
