#ifndef JSONDIFF_JSONDIFF_H
#define JSONDIFF_JSONDIFF_H

#include <jsondiff/config.h>
#include <jsondiff/diff_result.h>
#include <jsondiff/json_value_types.h>

#include <string>
#include <memory>

#include <fc/io/json.hpp>
#include <fc/string.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>


namespace jsondiff
{
	class JsonDiff
	{
	private:

	public:
		JsonDiff();
		virtual ~JsonDiff();


		DiffResultP diff_by_string(const std::string &old_json_str, const std::string &new_json_str);

		// 如果参数是json字符串，不要直接用下面这个函数，而是json_loads为json对象后调用diff函数，或者直接调用diff_by_string函数
		// @throws JsonDiffException
		DiffResultP diff(const JsonValue& old_json, const JsonValue& new_json);

		JsonValue patch_by_string(const std::string& old_json_value, DiffResultP diff_info);

		// 把旧版本的json,使用diff得到新版本
		// @throws JsonDiffException
		JsonValue patch(const JsonValue& old_json, const DiffResultP& diff_info);

		JsonValue rollback_by_string(const std::string& new_json_value, DiffResultP diff_info);

		// 从新版本使用diff回滚到旧版本
		// @throws JsonDiffException
		JsonValue rollback(const JsonValue& new_json, DiffResultP diff_info);

	};
}

#endif
