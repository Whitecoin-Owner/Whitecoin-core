#pragma once
#include <cborcpp/cbor.h>
#include <cbor_diff/cbor_diff_exceptions.h>

namespace cbor_diff {

#define CBORDIFF_KEY_ADDED_POSTFIX "__added"
#define CBORDIFF_KEY_DELETED_POSTFIX "__deleted"

#define CBORDIFF_KEY_OLD_VALUE "__old"
#define CBORDIFF_KEY_NEW_VALUE "__new"

	std::string cbor_to_hex(const cbor::CborObjectP value);
	std::string cbor_to_hex(const cbor::CborObject& value);
	cbor::CborObjectP cbor_from_hex(const std::string& hex_str);

	std::vector<char> cbor_encode(const cbor::CborObjectP value);
	std::vector<char> cbor_encode(const cbor::CborObject& value);
	cbor::CborObjectP cbor_decode(const std::vector<char>& input_bytes);

	cbor::CborObjectP cbor_deep_clone(cbor::CborObject* object);

	bool is_scalar_cbor_value_type(const cbor::CborObjectType& cbor_value_type);

	bool is_scalar_cbor_value_diff_format(const cbor::CborObject& diff_value);

	class DiffResult
	{
	private:
		cbor::CborObject _diff_value;
		bool _is_undefined;
	public:
		DiffResult();
		DiffResult(const cbor::CborObject& diff_value);
		virtual ~DiffResult();

		std::string str() const;
		std::string pretty_str() const;
		bool is_undefined() const;

		const cbor::CborObject& value() const;

		// 把 cbor diff转成友好可读的字符串
		std::string pretty_diff_str(size_t indent_count = 0) const;

	public:
		static std::shared_ptr<DiffResult> make_undefined_diff_result();
	};

	typedef std::shared_ptr<DiffResult> DiffResultP;

	class CborDiff {
	private:
	public:
		CborDiff() {}
		virtual ~CborDiff() {}

		DiffResultP diff_by_hex(const std::string &old_hex, const std::string &new_hex);

		// 如果参数是cbor hex字符串，不要直接用下面这个函数，而是cbor_decode为cbor对象后调用diff函数，或者直接调用diff_by_string函数
		// @throws CborDiffException
		DiffResultP diff(const cbor::CborObjectP old_val, const cbor::CborObjectP new_val);

		cbor::CborObjectP patch_by_string(const std::string& old_hex, DiffResultP diff_info);

		// 把旧版本的json,使用diff得到新版本
		// @throws CborDiffException
		cbor::CborObjectP patch(const cbor::CborObjectP old_val, const DiffResultP diff_info);

		cbor::CborObjectP rollback_by_string(const std::string& new_hex, DiffResultP diff_info);

		// 从新版本使用diff回滚到旧版本
		// @throws CborDiffException
		cbor::CborObjectP rollback(const cbor::CborObjectP new_val, DiffResultP diff_info);
	};
}
