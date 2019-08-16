#include "cborcpp/cbor_object.h"
#include <fc/crypto/hex.hpp>
#include <string>
#include <sstream>

namespace cbor {

	CborObject::CborObject()
	: type(COT_NULL), array_or_map_size(0), is_positive_extra(false){
		value.bool_val = 0;
	}

	CborObject::CborObject(const CborObject& other)
	: type(other.type), value(other.value), array_or_map_size(other.array_or_map_size), is_positive_extra(other.is_positive_extra) {

	}

	CborIntValue CborObject::force_as_int() const {
		if (is_int()) {
			return as_int();
		}
		else if (is_extra_int()) {
			auto extra_int_val = static_cast<CborIntValue>(as_extra_int());
			auto is_pos = this->is_positive_extra;
			if (is_pos) {
				return extra_int_val;
			}
			else {
				if (extra_int_val >= 0) {
					return -extra_int_val;
				}
				else {
					return extra_int_val;
				}
			}
		}
		else {
			FC_THROW_EXCEPTION(fc::assert_exception, "this cbor_object with can't be passed as int");
		}
	}

	fc::variant CborObject::to_json() const {
		switch (type) {
		case COT_NULL:
			return fc::variant();
		case COT_UNDEFINED:
			return fc::variant();
		case COT_BOOL:
			return as_bool() ? true : false;
		case COT_INT:
			return as_int();
		case COT_EXTRA_INT: {
			auto extra_int_val = as_extra_int();
			auto is_pos = this->is_positive_extra;
			if (is_pos)
				return extra_int_val;
			else {
				int64_t result_val = static_cast<int64_t>(extra_int_val);
				result_val = -result_val;
				return result_val;
			}
		} case COT_FLOAT:
			return as_float64();
		case COT_TAG:
			return as_tag();
		case COT_STRING:
			return as_string();
		case COT_BYTES: {
			const auto& bytes = as_bytes();
			const auto& hex = fc::to_hex(bytes);
			return hex;
		}
		case COT_ARRAY: {
			fc::variants arr;
			const auto& items = as_array();
			for (size_t i = 0; i < items.size(); i++) {
				arr.push_back(items[i]->to_json());
			}
			return arr;
		} break;
		case COT_MAP:
		{
			fc::mutable_variant_object obj;
			const auto& items = as_map();
			for (auto it = items.begin(); it != items.end(); it++) {
				obj[it->first] = it->second->to_json();
			}
			return obj;
		}
		default:
			throw cbor::CborException(std::string("not supported cbor object type ") + std::to_string(type));
		}
	}

	std::string CborObject::str() const {
		switch (type) {
		case COT_NULL:
			return "null";
		case COT_UNDEFINED:
			return "undefined";
		case COT_BOOL:
			return as_bool() ? "true" : "false";
		case COT_INT:
			return std::to_string(as_int());
		case COT_EXTRA_INT: {
			auto extra_int_val = as_extra_int();
			auto is_pos = this->is_positive_extra;
			if (is_pos)
				return std::to_string(extra_int_val);
			else
				return std::string("-") + std::to_string(extra_int_val);
		} case COT_FLOAT:
			return std::to_string(as_float64());
		case COT_TAG:
			return std::string("tag<") + std::to_string(as_tag()) + ">";
		case COT_STRING:
			return as_string();
		case COT_BYTES: {
			const auto& bytes = as_bytes();
			const auto& hex = fc::to_hex(bytes);
			return std::string("bytes<") + hex + ">";
		}
		case COT_ARRAY: {
			const auto& items = as_array();
			std::stringstream ss;
			ss << "[";
			for (size_t i = 0; i < items.size(); i++) {
				if (i > 0) {
					ss << ", ";
				}
				ss << items[i]->str();
			}
			ss << "]";
			return ss.str();
		} break;
		case COT_MAP:
		{
			const auto& items = as_map();
			std::stringstream ss;
			ss << "{";
			auto begin = items.begin();
			for (auto it = items.begin(); it != items.end();it++) {
				if (it != begin) {
					ss << ", ";
				}
				ss << it->first << ": " << it->second->str();
			}
			ss << "}";
			return ss.str();
		}
		default:
			throw cbor::CborException(std::string("not supported cbor object type ") + std::to_string(type));
		}
	}

	/*CborObjectP CborObject::from(const CborObjectValue& value) {
		auto result = std::make_shared<CborObject>();
		result->value = value;
		switch (value.which()) {
		case CborObjectValue::tag<CborBoolValue>::value: 
			result->type = COT_BOOL;
			break;
		case CborObjectValue::tag<CborIntValue>::value:
			result->type = COT_INT;
			break;
		case CborObjectValue::tag<CborExtraIntValue>::value:
			result->type = COT_EXTRA_INT;
			break;
		case CborObjectValue::tag<CborDoubleValue>::value:
			result->type = COT_FLOAT;
			break;
		case CborObjectValue::tag<CborTagValue>::value:
			result->type = COT_TAG;
			break;
		case CborObjectValue::tag<CborStringValue>::value:
			result->type = COT_STRING;
			break;
		case CborObjectValue::tag<CborBytesValue>::value:
			result->type = COT_BYTES;
			break;
		case CborObjectValue::tag<CborArrayValue>::value:
			result->type = COT_ARRAY;
			break;
		case CborObjectValue::tag<CborMapValue>::value:
			result->type = COT_MAP;
			break;
		default:
			throw cbor::CborException(std::string("not supported cbor object type ") + std::to_string(value.which()));
		}
		return result;
	}*/

	CborObjectP CborObject::from_int(CborIntValue value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_INT;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.int_val = value;
#endif
		return result;
	}
	CborObjectP CborObject::from_bool(CborBoolValue value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_BOOL;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.bool_val = value;
#endif
		return result;
	}
	CborObjectP CborObject::from_bytes(const CborBytesValue& value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_BYTES;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.bytes_val = value;
#endif
		return result;
	}
	CborObjectP CborObject::from_float64(const CborDoubleValue& value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_FLOAT;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.float64_val = value;
#endif
		return result;
	}
	CborObjectP CborObject::from_string(const std::string& value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_STRING;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.string_val = value;
#endif
		return result;
	}
	CborObjectP CborObject::create_array(size_t size) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_ARRAY;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = CborArrayValue();
#else
		result->value.array_val = CborArrayValue();
#endif
		result->array_or_map_size = size;
		return result;
	}
	CborObjectP CborObject::create_array(const CborArrayValue& items) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_ARRAY;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = CborArrayValue(items);
#else
		result->value.array_val = CborArrayValue(items);
#endif
		result->array_or_map_size = items.size();
		return result;
	}
	CborObjectP CborObject::create_map(size_t size) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_MAP;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = CborMapValue();
#else
		result->value.map_val = CborMapValue();
#endif
		result->array_or_map_size = size;
		return result;
	}
	CborObjectP CborObject::create_map(const CborMapValue& items) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_MAP;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = CborMapValue(items);
#else
		result->value.map_val = CborMapValue(items);
#endif
		result->array_or_map_size = items.size();
		return result;
	}
	CborObjectP CborObject::from_tag(CborTagValue value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_TAG;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.tag_or_special_val = value;
#endif
		return result;
	}
	CborObjectP CborObject::create_undefined() {
		auto result = std::make_shared<CborObject>();
		result->type = COT_UNDEFINED;
#if !CBOR_OBJECT_USE_VARIANT
		result->value.bool_val = 0;
#endif
		return result;
	}
	CborObjectP CborObject::create_null() {
		auto result = std::make_shared<CborObject>();
		result->type = COT_NULL;
#if !CBOR_OBJECT_USE_VARIANT
		result->value.bool_val = 0;
#endif
		return result;
	}
	/*CborObjectP CborObject::from_special(uint32_t value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_SPECIAL;
		result->value = value;
		return result;
	}*/
	CborObjectP CborObject::from_extra_integer(uint64_t value, bool sign) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_EXTRA_INT;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.extra_int_val = value;
#endif
		result->is_positive_extra = sign;
		return result;
	}
	CborObjectP CborObject::from_extra_tag(uint64_t value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_EXTRA_TAG;
#if defined(CBOR_OBJECT_USE_VARIANT)
		result->value = value;
#else
		result->value.extra_int_val = value;
#endif
		return result;
	}
	/*CborObjectP CborObject::from_extra_special(uint64_t value) {
		auto result = std::make_shared<CborObject>();
		result->type = COT_EXTRA_SPECIAL;
		result->value = value;
		return result;
	}*/
}
