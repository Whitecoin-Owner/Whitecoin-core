#ifndef JSONDIFF_JSON_VALUE_TYPES_H
#define JSONDIFF_JSON_VALUE_TYPES_H

#include <jsondiff/config.h>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

namespace jsondiff
{
	enum JsonValueType
	{
		JVT_NULL = 0,
		JVT_INTEGER = 1,
		JVT_FLOAT = 2,
		JVT_BOOLEAN = 3,
		JVT_STRING = 4,
		JVT_OBJECT = 5,
		JVT_ARRAY = 6,

		JVT_UNDEFINED = 10
	};

	typedef fc::variant JsonValue;

	typedef fc::variants JsonArray;

	typedef fc::mutable_variant_object JsonObject;

	inline bool is_scalar_json_value_type(const JsonValueType& json_value_type)
	{
		return json_value_type == JVT_NULL || json_value_type == JVT_INTEGER
			|| json_value_type == JVT_FLOAT || json_value_type == JVT_BOOLEAN
			|| json_value_type == JVT_STRING || json_value_type == JVT_UNDEFINED;
	}

	// @throws JsonDiffException
	JsonValueType guess_json_value_type(const JsonValue& json_data);
	// @throws JsonDiffException
	JsonValue json_loads(const std::string& json_str);
	std::string json_dumps(const JsonValue& json_value);
	std::string json_pretty_dumps(const JsonValue& json_value);

	JsonValue json_deep_clone(const JsonValue& json_value);

	bool json_has_key(const JsonObject& json_value, std::string key);

	bool is_scalar_value_diff_format(const JsonValue& diff_json);
}

#endif
