#pragma once

#include <boost/variant.hpp>
#include <fc/variant.hpp>
#include <fc/static_variant.hpp>
#include <fc/variant_object.hpp>
#include <fc/variant.hpp>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <cborcpp/exceptions.h>

namespace cbor {
	enum CborObjectType {
		COT_BOOL,
		COT_INT,
		COT_BYTES,
		COT_STRING,
		COT_ARRAY,
		COT_MAP,
		COT_TAG,

		COT_FLOAT,
		// COT_SPECIAL,

		COT_UNDEFINED,
		COT_NULL,
		COT_ERROR,
		COT_EXTRA_INT,
		COT_EXTRA_TAG,
		//COT_EXTRA_SPECIAL,

	};
	struct CborObject;
	typedef std::shared_ptr<CborObject> CborObjectP;
	typedef bool CborBoolValue;
	typedef int64_t CborIntValue;
	typedef uint32_t CborTagValue;
	typedef double CborDoubleValue;
	typedef std::vector<char> CborBytesValue;
	typedef std::vector<CborObjectP> CborArrayValue;
	typedef std::map<std::string, CborObjectP, std::less<std::string>> CborMapValue;
	typedef std::string CborStringValue;
	typedef std::string CborErrorValue;
	typedef uint64_t CborExtraIntValue;
	typedef uint32_t CborSpecialValue;
	//typedef uint64_t CborExtraSpecialValue;

#define CBOR_ENCODE_DOUBLE_STRING_SIZE 40

	// #define CBOR_OBJECT_USE_VARIANT

#if defined(CBOR_OBJECT_USE_VARIANT)
	typedef fc::static_variant<CborBoolValue, CborIntValue, CborExtraIntValue, CborDoubleValue, CborTagValue, CborStringValue, CborBytesValue,
		CborArrayValue, CborMapValue> CborObjectValue;
#else
	typedef struct _CborObjectValue { // union if allow copy constrctor in union
		CborBoolValue bool_val;
		CborIntValue int_val;
		CborExtraIntValue extra_int_val;
		CborDoubleValue float64_val;
		CborTagValue tag_or_special_val;
		CborStringValue string_val;
		CborBytesValue bytes_val;
		CborArrayValue array_val;
		CborMapValue map_val;
	} CborObjectValue;
#endif

	struct CborObject {
		CborObjectType type;
		CborObjectValue value;
		uint32_t array_or_map_size = 0;
		bool is_positive_extra = false;

		CborObject();
		CborObject(const CborObject& other);
		~CborObject() {}

#if defined(CBOR_OBJECT_USE_VARIANT)
		template <typename T>
		const T& as() const {
			return value.get<T>();
		}
#endif

		inline bool is_null() const {
			return COT_NULL == type;
		}
		inline bool is_undefined() const {
			return COT_UNDEFINED == type;
		}
		inline bool is_int() const {
			return COT_INT == type;
		}
		inline bool is_extra_int() const {
			return COT_EXTRA_INT == type;
		}
		inline bool is_integer() const {
			return is_int() || is_extra_int();
		}
		inline bool is_string() const {
			return COT_STRING == type;
		}
		inline bool is_float() const {
			return COT_FLOAT == type;
		}
		inline bool is_bytes() const {
			return COT_BYTES == type;
		}
		inline bool is_bool() const {
			return COT_BOOL == type;
		}
		inline bool is_array() const {
			return COT_ARRAY == type;
		}
		inline bool is_map() const {
			return COT_MAP == type;
		}
		inline bool is_tag() const {
			return COT_TAG == type;
		}
		/*inline bool is_special() const {
			return COT_SPECIAL == type;
		}*/
		inline CborObjectType object_type() const {
			return type;
		}
		inline const CborBoolValue as_bool() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborBoolValue>();
#else
			return value.bool_val;
#endif
		}
		inline const CborIntValue as_int() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborIntValue>();
#else
			return value.int_val;
#endif
		}
		inline const CborExtraIntValue as_extra_int() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborExtraIntValue>();
#else
			return value.extra_int_val;
#endif
		}
		CborIntValue force_as_int() const;

		inline const CborTagValue as_tag() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborTagValue>();
#else
			return value.tag_or_special_val;
#endif
		}
		inline const CborExtraIntValue as_extra_tag() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborExtraIntValue>();
#else
			return value.extra_int_val;
#endif
		}
		inline const CborSpecialValue& as_special() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborSpecialValue>();
#else
			return value.tag_or_special_val;
#endif
		}
		inline const CborBytesValue& as_bytes() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborBytesValue>();
#else
			return value.bytes_val;
#endif
		}
		inline const CborArrayValue& as_array() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborArrayValue>();
#else
			return value.array_val;
#endif
		}
		inline const CborMapValue& as_map() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborMapValue>();
#else
			return value.map_val;
#endif
		}
		inline const CborStringValue& as_string() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborStringValue>();
#else
			return value.string_val;
#endif
		}
		inline const CborDoubleValue as_float64() const {
#if defined(CBOR_OBJECT_USE_VARIANT)
			return as<CborDoubleValue>();
#else
			return value.float64_val;
#endif
		}

		std::string str() const;

		fc::variant to_json() const;

		//static CborObjectP from(const CborObjectValue& value);

		static CborObjectP from_int(CborIntValue value);
		static CborObjectP from_bool(CborBoolValue value);
		static CborObjectP from_bytes(const CborBytesValue& value);
		static CborObjectP from_float64(const CborDoubleValue& value);
		static CborObjectP from_string(const std::string& value);
		static CborObjectP create_array(size_t size);
		static CborObjectP create_array(const CborArrayValue& items);
		static CborObjectP create_map(size_t size);
		static CborObjectP create_map(const CborMapValue& items);
		static CborObjectP from_tag(CborTagValue value);
		static CborObjectP create_undefined();
		static CborObjectP create_null();
		// static CborObjectP from_special(uint32_t value);
		static CborObjectP from_extra_integer(uint64_t value, bool sign);
		static CborObjectP from_extra_tag(uint64_t value);
		// static CborObjectP from_extra_special(uint64_t value);
	};
}
