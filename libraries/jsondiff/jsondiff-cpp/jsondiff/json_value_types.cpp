#include <jsondiff/json_value_types.h>
#include <jsondiff/exceptions.h>

namespace jsondiff
{
	JsonValueType guess_json_value_type(const JsonValue& json_data)
	{
		if (json_data.is_null())
			return JsonValueType::JVT_NULL;
		if (json_data.is_object())
			return JsonValueType::JVT_OBJECT;
		if (json_data.is_array())
			return JsonValueType::JVT_ARRAY;
		if (json_data.is_string())
			return JsonValueType::JVT_STRING;
		if (json_data.is_bool())
			return JsonValueType::JVT_BOOLEAN;
		if (json_data.is_numeric() && !json_data.is_integer())
			return JsonValueType::JVT_FLOAT;
		if (json_data.is_integer())
			return JsonValueType::JVT_INTEGER;
		throw JsonDiffException(std::string("don't known what json type of ") + json_dumps(json_data) + " is");
	}


	std::string json_dumps(const JsonValue& json_value)
	{
		return fc::json::to_string(json_value, fc::json::legacy_generator);
	}

	std::string json_pretty_dumps(const JsonValue& json_value)
	{
		return fc::json::to_pretty_string(json_value, fc::json::legacy_generator);
	}

	JsonValue json_deep_clone(const JsonValue& json_value)
	{
		return json_loads(json_dumps(json_value));
	}

	JsonValue json_loads(const std::string& json_str)
	{
		try
		{
			return fc::json::from_string(json_str, fc::json::legacy_parser);
		}
		catch (std::exception &e)
		{
			throw JsonDiffException(e.what());
		}
	}

	bool json_has_key(const JsonObject& json_value, std::string key)
	{
		return json_value.find(key) != json_value.end();
	}

	bool is_scalar_value_diff_format(const JsonValue& diff_json)
	{
		if (!diff_json.is_object())
			return false;
		auto diff_json_obj = diff_json.as<fc::mutable_variant_object>();
		return json_has_key(diff_json_obj, JSONDIFF_KEY_OLD_VALUE) && json_has_key(diff_json_obj, JSONDIFF_KEY_NEW_VALUE);
	}
}
