#include <cbor_diff/cbor_diff.h>
#include <cbor_diff/helper.h>
#include <uvm/uvm_lutil.h>
#include <fc/crypto/hex.hpp>

namespace cbor_diff {

	bool is_scalar_cbor_value_type(const cbor::CborObjectType& cbor_value_type)
	{
		using namespace cbor;
		return cbor_value_type == COT_BOOL || cbor_value_type == COT_INT || cbor_value_type == COT_EXTRA_INT
			|| cbor_value_type == COT_BYTES || cbor_value_type == COT_STRING || cbor_value_type == COT_FLOAT
			|| cbor_value_type == COT_TAG || cbor_value_type == COT_EXTRA_TAG || cbor_value_type == COT_NULL || cbor_value_type == COT_UNDEFINED;
	}

	bool is_scalar_cbor_value_diff_format(const cbor::CborObject& diff_value)
	{
		if (!diff_value.is_map())
			return false;
		const auto& diff_map_obj = diff_value.as_map();
		return diff_map_obj.find(CBORDIFF_KEY_OLD_VALUE) != diff_map_obj.end() && diff_map_obj.find(CBORDIFF_KEY_NEW_VALUE) != diff_map_obj.end();
	}


	std::string cbor_to_hex(const cbor::CborObject& value) {
		cbor::output_dynamic output;
		cbor::encoder encoder(output);
		encoder.write_cbor_object(&value);
		const auto& output_hex = output.hex();
		return output_hex;
	}
	std::string cbor_to_hex(const cbor::CborObjectP value) {
		cbor::output_dynamic output;
		cbor::encoder encoder(output);
		encoder.write_cbor_object(value.get());
		const auto& output_hex = output.hex();
		return output_hex;
	}

	cbor::CborObjectP cbor_from_hex(const std::string& hex_str) {
		std::vector<char> input_bytes(hex_str.size() / 2);
		if (fc::from_hex(hex_str, input_bytes.data(), input_bytes.size()) < input_bytes.size())
			throw CborDiffException("invalid hex string");
		cbor::input input(input_bytes.data(), input_bytes.size());
		cbor::decoder decoder(input);
		auto result_cbor = decoder.run();
		return result_cbor;
	}

	std::vector<char> cbor_encode(const cbor::CborObject& value) {
		cbor::output_dynamic output;
		cbor::encoder encoder(output);
		encoder.write_cbor_object(&value);
		const auto& output_bytes = output.chars();
		return output_bytes;
	}
	std::vector<char> cbor_encode(const cbor::CborObjectP value) {
		cbor::output_dynamic output;
		cbor::encoder encoder(output);
		encoder.write_cbor_object(value.get());
		const auto& output_bytes = output.chars();
		return output_bytes;
	}

	cbor::CborObjectP cbor_decode(const std::vector<char>& input_bytes) {
		std::vector<char> source(input_bytes);
		cbor::input input(source.data(), source.size());
		cbor::decoder decoder(input);
		auto result_cbor = decoder.run();
		return result_cbor;
	}

	cbor::CborObjectP cbor_deep_clone(cbor::CborObject* object) {
		return cbor_decode(cbor_encode(*object));
	}

	DiffResult::DiffResult()
		: _is_undefined(true) {

	}

	DiffResult::DiffResult(const cbor::CborObject& diff_value)
		: _diff_value(diff_value)
	{
		if (diff_value.is_null())
			_is_undefined = true;
		else
			_is_undefined = false;
	}

	DiffResult::~DiffResult() {

	}

	std::string DiffResult::str() const {
		if (_is_undefined) {
			return "undefined";
		}
		else {
			return _diff_value.str();
		}
	}
	std::string DiffResult::pretty_str() const {
		return pretty_diff_str(2);
	}

	bool DiffResult::is_undefined() const {
		return _is_undefined;
	}

	const cbor::CborObject& DiffResult::value() const {
		return _diff_value;
	}

	// 把 cbor diff转成友好可读的字符串
	std::string DiffResult::pretty_diff_str(size_t indent_count) const {
		// TODO: indents
		std::stringstream indent_ss;
		for (size_t i = 0; i < indent_count; i++)
			indent_ss << "\t";
		auto indents = indent_ss.str();
		auto diff_cbor_type = _diff_value.type;
		std::stringstream ss;
		if (is_scalar_cbor_value_type(diff_cbor_type))
		{
			// 基本类型
			ss << indents << " " << _diff_value.str();
			return ss.str();
		}
		else if (diff_cbor_type == cbor::COT_MAP)
		{
			const auto& diff_map_obj = _diff_value.as_map();
			if (is_scalar_cbor_value_diff_format(_diff_value))
			{
				// 具有 {__old: ..., __new: ...}格式时
				ss << indents << "-" << diff_map_obj.at(CBORDIFF_KEY_OLD_VALUE)->str() << std::endl;
				ss << indents << "+" << diff_map_obj.at(CBORDIFF_KEY_NEW_VALUE)->str() << std::endl;
				return ss.str();
			}
			for (auto i = diff_map_obj.begin(); i != diff_map_obj.end(); i++)
			{
				auto key = i->first;
				auto diff_item = i->second;
				// 如果key是 <key>__deleted 或者 <key>__added，则是删除或者添加，否则是修改现有key的值
				if (utils::string_ends_with(key, CBORDIFF_KEY_ADDED_POSTFIX) && key.size() > strlen(CBORDIFF_KEY_ADDED_POSTFIX))
				{
					auto origin_key = utils::string_without_ext(key, CBORDIFF_KEY_ADDED_POSTFIX);
					ss << indents << "\t+" << origin_key << ":" << diff_item->str() << std::endl;
					continue;
				}
				else if (utils::string_ends_with(key, CBORDIFF_KEY_DELETED_POSTFIX) && key.size() > strlen(CBORDIFF_KEY_DELETED_POSTFIX))
				{
					auto origin_key = utils::string_without_ext(key, CBORDIFF_KEY_DELETED_POSTFIX);
					ss << indents << "\t-" << origin_key << ":" << diff_item->str() << std::endl;
					continue;
				}
				// 修改现有key的值
				ss << indents << "\t" << key << ":" << std::endl << std::make_shared<DiffResult>(*diff_item)->pretty_diff_str(indent_count + 1) << std::endl;
			}
			return ss.str();
		}
		else if (diff_cbor_type == cbor::COT_ARRAY)
		{
			const auto& diff_obj_array = _diff_value.as_array();
			for (size_t i = 0; i < diff_obj_array.size(); i++)
			{
				if (!diff_obj_array[i]->is_array())
				{
					ss << indents << "\t " << diff_obj_array[i]->str() << std::endl;
					continue;
				}
				auto diff_item = diff_obj_array[i]->as_array();
				if (diff_item.size() != 3)
				{
					ss << indents << "\t " << diff_obj_array[i]->str() << std::endl;
					continue;
				}
				auto op_item = diff_item[0]->as_string();
				auto pos = diff_item[1]->force_as_int();
				auto inner_diff_json = diff_item[2];
				// FIXME； 一个array有多项变化的时候， diff里的索引是用原始对象的index，所以这里应该找出 pos => old_json中同值的pos
				if (op_item == std::string("+"))
				{
					// 添加元素
					ss << indents << "\t+" << inner_diff_json->str() << std::endl;
				}
				else if (op_item == std::string("-"))
				{
					// 删除元素
					ss << indents << "\t-" << inner_diff_json->str() << std::endl;
				}
				else if (op_item == std::string("~"))
				{
					// 修改元素
					ss << indents << "\t~" << std::endl << std::make_shared<DiffResult>(*inner_diff_json)->pretty_diff_str(indent_count + 1) << std::endl;
				}
				else
				{
					ss << indents << "\t " << diff_obj_array[i]->str() << std::endl;
					continue;
				}
			}
			return ss.str();
		}
		else
		{
			ss << indents << _diff_value.str();
			return ss.str();
		}
	}

	std::shared_ptr<DiffResult> DiffResult::make_undefined_diff_result() {
		auto result = std::make_shared<DiffResult>();
		result->_is_undefined = true;
		result->_diff_value = *cbor::CborObject::create_undefined();
		return result;
	}


	// CborDiff

	DiffResultP CborDiff::diff_by_hex(const std::string &old_hex, const std::string &new_hex) {
		return diff(cbor_from_hex(old_hex), cbor_from_hex(new_hex));
	}

	// 如果参数是cbor hex字符串，不要直接用下面这个函数，而是cbor_decode为cbor对象后调用diff函数，或者直接调用diff_by_string函数
	// @throws CborDiffException
	DiffResultP CborDiff::diff(const cbor::CborObjectP old_val, const cbor::CborObjectP new_val) {
		auto old_json_type = old_val->object_type();
		auto new_json_type = new_val->object_type();

		if (is_scalar_cbor_value_type(old_json_type) || old_json_type != new_json_type)
		{
			// old值是基本类型 或old和new值的类型不一样
			// should return undefined for two identical values
			// should return { __old: <old value>, __new : <new value> } object for two different numbers
			if (is_scalar_cbor_value_type(old_json_type) && old_json_type == new_json_type) {
				if (cbor::COT_STRING == old_json_type) {
					const auto& old_str = old_val->as_string();
					const auto& new_str = new_val->as_string();
					if (old_str.size() != new_str.size() || old_str != new_str) {
						cbor::CborMapValue result_json;
						result_json[CBORDIFF_KEY_OLD_VALUE] = old_val;
						result_json[CBORDIFF_KEY_NEW_VALUE] = new_val;
						return std::make_shared<DiffResult>(*cbor::CborObject::create_map(result_json));
					}
					else {
						// identical scalar values
						return DiffResult::make_undefined_diff_result();
					}
				}
				else if (cbor_encode(old_val) != cbor_encode(new_val)) {
					cbor::CborMapValue result_json;
					result_json[CBORDIFF_KEY_OLD_VALUE] = old_val;
					result_json[CBORDIFF_KEY_NEW_VALUE] = new_val;
					return std::make_shared<DiffResult>(*cbor::CborObject::create_map(result_json));
				}
				else {
					// identical scalar values
					return DiffResult::make_undefined_diff_result();
				}
			}
			else if (old_json_type != new_json_type)
			{
				cbor::CborMapValue result_json;
				result_json[CBORDIFF_KEY_OLD_VALUE] = old_val;
				result_json[CBORDIFF_KEY_NEW_VALUE] = new_val;
				return std::make_shared<DiffResult>(*cbor::CborObject::create_map(result_json));
			}
			else
			{
				// identical scalar values
				return DiffResult::make_undefined_diff_result();
			}
		}
		else if (old_json_type == cbor::COT_MAP)
		{
			// old和new都是object类型时
			// should return undefined for two objects with identical contents
			// should return undefined for two object hierarchies with identical contents
			// should return { <key>__deleted: <old value> } when the second object is missing a key
			// should return { <key>__added: <new value> } when the first object is missing a key
			// should return { <key>: { __old: <old value>, __new : <new value> } } for two objects with diffent scalar values for a key
			// should return { <key>: <diff> } with a recursive diff for two objects with diffent values for a key
			const auto& a_obj = old_val->as_map();
			const auto& b_obj = new_val->as_map();
			bool same = false;
			cbor::CborMapValue diff_json;
			for (auto i = a_obj.begin(); i != a_obj.end(); i++)
			{
				auto a_i_value = i->second;
				auto a_i_key = i->first;
				if (b_obj.find(a_i_key) == b_obj.end())
				{
					// 存在于old不存在于new
					diff_json[a_i_key + CBORDIFF_KEY_DELETED_POSTFIX] = a_i_value;
				}
				else
				{
					// old和new中都有这个key
					auto sub_diff_value = diff(a_i_value, b_obj.at(a_i_key));
					if (sub_diff_value->is_undefined()) // 一样的元素
						continue;
					// 修改
					diff_json[a_i_key] = std::make_shared<cbor::CborObject>(sub_diff_value->value());
				}
			}
			for (auto j = b_obj.begin(); j != b_obj.end(); j++)
			{
				auto key = j->first;
				if (a_obj.find(key) == a_obj.end())
				{
					// 不存在于old但是存在于new
					diff_json[key + CBORDIFF_KEY_ADDED_POSTFIX] = j->second;
				}
			}
			if (diff_json.size() < 1)
				return DiffResult::make_undefined_diff_result();
			return std::make_shared<DiffResult>(*cbor::CborObject::create_map(diff_json));
		}
		else if (old_json_type == cbor::COT_ARRAY)
		{
			// old和new都是array类型时
			// with arrays of scalars
			//   should return undefined for two arrays with identical contents
			//   should return[..., ['-', remove_from_position_index, <removed item>], ...] for two arrays when the second array is missing a value
			//   should return[..., ['+', insert_position_index, <added item>], ...] for two arrays when the second one has an extra value
			//   should return[..., ['+', insert_position_index, <added item>]] for two arrays when the second one has an extra value at the end(edge case test)
			// with arrays of objects
			//   should return undefined for two arrays with identical contents
			//   should return[..., ['-', remove_from_position_index, <removed item>], ...] for two arrays when the second array is missing a value
			//   should return[..., ['+', insert_position_index, <added item>], ...] for two arrays when the second array has an extra value
			//   should return[..., ['~', position_index, <diff>], ...] for two arrays when an item has been modified(note: involves a crazy heuristic)

			const auto& a_array = old_val->as_array();
			const auto& b_array = new_val->as_array();

			// TODO: 当两个array的大部分元素相同时，但是可能前方插入部分元素，这时候应该尽量减少diff大小

			// 一个array有多项变化的时候， diff里的索引是用原始对象的index

			cbor::CborArrayValue diff_json;
			for (size_t i = 0; i < a_array.size(); i++)
			{
				if (i >= b_array.size())
				{
					// 删除元素
					cbor::CborArrayValue item_diff;
					item_diff.push_back(cbor::CborObject::from_string("-"));
					item_diff.push_back(cbor::CborObject::from_int(i));
					item_diff.push_back(a_array[i]);
					diff_json.push_back(cbor::CborObject::create_array(item_diff));
				}
				else
				{
					auto item_value_diff = diff(a_array[i], b_array[i]);
					if (item_value_diff->is_undefined()) // 没有发生改变
						continue;
					// 修改元素
					cbor::CborArrayValue item_diff;
					item_diff.push_back(cbor::CborObject::from_string("~"));
					item_diff.push_back(cbor::CborObject::from_int(i));
					item_diff.push_back(std::make_shared<cbor::CborObject>(item_value_diff->value()));
					diff_json.push_back(cbor::CborObject::create_array(item_diff));
				}
			}
			for (size_t i = a_array.size(); i < b_array.size(); i++)
			{
				// 不存在于old但是存在于new中
				cbor::CborArrayValue item_diff;
				item_diff.push_back(cbor::CborObject::from_string("+"));
				item_diff.push_back(cbor::CborObject::from_int(i));
				item_diff.push_back(b_array[i]);
				diff_json.push_back(cbor::CborObject::create_array(item_diff));
			}
			if (diff_json.size() < 1)
			{
				return DiffResult::make_undefined_diff_result();
			}
			return std::make_shared<DiffResult>(*cbor::CborObject::create_array(diff_json));
		}
		else
		{
			throw CborDiffException(std::string("not supported json value type to diff ") + old_val->str());
		}
	}

	cbor::CborObjectP CborDiff::patch_by_string(const std::string& old_hex, DiffResultP diff_info) {
		auto old_val = cbor_from_hex(old_hex);
		return patch(old_val, diff_info);
	}

	// 把旧版本的json,使用diff得到新版本
	// @throws CborDiffException
	cbor::CborObjectP CborDiff::patch(const cbor::CborObjectP old_val, const DiffResultP diff_info) {
		auto old_json_type = old_val->object_type();
		const auto& diff_json = diff_info->value();
		auto result = cbor_deep_clone(old_val.get());
		if (diff_info->is_undefined() || diff_json.is_null())
			return result;


		if (is_scalar_cbor_value_type(old_json_type) || is_scalar_cbor_value_diff_format(diff_json))
		{ // TODO: 这个判断要修改得简单准确一点，修改diffjson格式，区分{__old: ..., __new: ...}和普通object diff
			if (!diff_json.is_map())
				throw CborDiffException("wrong format of diffjson of scalar json value");
			const auto& diff_map = diff_json.as_map();
			result = diff_map.at(CBORDIFF_KEY_NEW_VALUE);
		}
		else if (old_json_type == cbor::COT_MAP)
		{
			const auto& old_json_str = cbor_encode(old_val);
			const auto& diff_json_str = cbor_encode(diff_json);
			const auto& old_json_obj = old_val->as_map();
			const auto& diff_json_obj = diff_json.as_map();
			auto result_obj = result->as_map();
			for (auto i = diff_json_obj.begin(); i != diff_json_obj.end(); i++)
			{
				auto key = i->first;
				auto diff_item = i->second;
				// 如果key是 <key>__deleted 或者 <key>__added，则是删除或者添加，否则是修改现有key的值
				if (utils::string_ends_with(key, CBORDIFF_KEY_DELETED_POSTFIX) && key.size() > strlen(CBORDIFF_KEY_DELETED_POSTFIX))
				{
					auto old_key = utils::string_without_ext(key, CBORDIFF_KEY_DELETED_POSTFIX);
					if (old_json_obj.find(old_key) != old_json_obj.end())
					{
						// 是删除属性操作
						result_obj.erase(old_key);
						continue;
					}
				}
				else if (utils::string_ends_with(key, CBORDIFF_KEY_ADDED_POSTFIX) && key.size() > strlen(CBORDIFF_KEY_ADDED_POSTFIX))
				{
					auto old_key = utils::string_without_ext(key, CBORDIFF_KEY_ADDED_POSTFIX);
					// 是增加属性操作
					result_obj[old_key] = diff_item;
					continue;
				}
				// 可能是修改现有key的值
				if (old_json_obj.find(key) == old_json_obj.end())
					throw CborDiffException("wrong format of diffjson of this old version json");
				auto sub_item_new = patch(old_json_obj.at(key), std::make_shared<DiffResult>(*diff_item));
				result_obj[key] = sub_item_new;
			}
			return cbor::CborObject::create_map(result_obj);
		}
		else if (old_json_type == cbor::COT_ARRAY)
		{
			const auto& old_json_str = cbor_encode(old_val);
			const auto& diff_json_str = cbor_encode(diff_json);
			const auto& old_json_array = old_val->as_array();
			const auto& diff_json_array = diff_json.as_array();
			auto result_array = result->as_array();
			for (size_t i = 0; i < diff_json_array.size(); i++)
			{
				if (!diff_json_array[i]->is_array())
					throw CborDiffException("diffjson format error for array diff");
				const auto& diff_item = diff_json_array[i]->as_array();
				if (diff_item.size() != 3)
					throw CborDiffException("diffjson format error for array diff");
				auto op_item = diff_item[0]->as_string();
				auto pos = diff_item[1]->force_as_int();
				auto inner_diff_json = diff_item[2];
				// FIXME； 一个array有多项变化的时候， diff里的索引是用原始对象的index，所以这里应该找出 pos => old_json中同值的pos
				if (op_item == std::string("+"))
				{
					// 添加元素
					result_array.insert(result_array.begin() + pos, inner_diff_json);
				}
				else if (op_item == std::string("-"))
				{
					// 删除元素
					result_array.erase(result_array.begin() + pos);
				}
				else if (op_item == std::string("~"))
				{
					// 修改元素
					auto sub_item_new = patch(old_json_array[i], std::make_shared<DiffResult>(*inner_diff_json));
					result_array[pos] = sub_item_new;
				}
				else
				{
					throw CborDiffException(std::string("not supported diff array op now: ") + op_item);
				}
			}
			return cbor::CborObject::create_array(result_array);
		}
		else
		{
			throw CborDiffException(std::string("not supported json value type to merge patch ") + old_val->str());
		}
		return result;
	}

	cbor::CborObjectP CborDiff::rollback_by_string(const std::string& new_hex, DiffResultP diff_info) {
		return rollback(cbor_from_hex(new_hex), diff_info);
	}

	// 从新版本使用diff回滚到旧版本
	// @throws CborDiffException
	cbor::CborObjectP CborDiff::rollback(const cbor::CborObjectP new_val, DiffResultP diff_info) {
		auto new_json_type = new_val->object_type();
		auto diff_json = diff_info->value();
		auto result = cbor_deep_clone(new_val.get());
		if (diff_info->is_undefined() || diff_json.is_null())
			return result;


		if (is_scalar_cbor_value_type(new_json_type) || is_scalar_cbor_value_diff_format(diff_json))
		{ // TODO: 这个判断要修改得简单准确一点，修改diffjson格式，区分{__old: ..., __new: ...}和普通object diff
			if (!diff_json.is_map())
				throw CborDiffException("wrong format of diffjson of scalar json value");
			const auto& diff_map = diff_json.as_map();
			result = diff_map.at(CBORDIFF_KEY_OLD_VALUE);
		}
		else if (new_json_type == cbor::COT_MAP)
		{
			const auto& new_json_str = cbor_encode(new_val);
			const auto& diff_json_str = cbor_encode(diff_json);
			const auto& new_json_obj = new_val->as_map();
			const auto& diff_json_obj = diff_json.as_map();
			cbor::CborMapValue result_obj = result->as_map();
			for (auto i = diff_json_obj.begin(); i != diff_json_obj.end(); i++)
			{
				auto key = i->first;
				auto diff_item = i->second;
				// 如果key是 <key>__deleted 或者 <key>__added，则是删除或者添加，否则是修改现有key的值
				if (utils::string_ends_with(key, CBORDIFF_KEY_ADDED_POSTFIX) && key.size() > strlen(CBORDIFF_KEY_ADDED_POSTFIX))
				{
					auto origin_key = utils::string_without_ext(key, CBORDIFF_KEY_ADDED_POSTFIX);
					if (new_json_obj.find(origin_key) != new_json_obj.end())
					{
						// 是增加属性操作，需要回滚
						result_obj.erase(origin_key);
						continue;
					}
				}
				else if (utils::string_ends_with(key, CBORDIFF_KEY_DELETED_POSTFIX) && key.size() > strlen(CBORDIFF_KEY_DELETED_POSTFIX))
				{
					auto origin_key = utils::string_without_ext(key, CBORDIFF_KEY_DELETED_POSTFIX);
					// 是删除属性操作，需要回滚
					result_obj[origin_key] = diff_item;
					continue;
				}
				// 可能是修改现有key的值
				if (new_json_obj.find(key) == new_json_obj.end())
					throw CborDiffException("wrong format of diffjson of this old version json");
				auto sub_item_new = rollback(new_json_obj.at(key), std::make_shared<DiffResult>(*diff_item));
				result_obj[key] = sub_item_new;
			}
			return cbor::CborObject::create_map(result_obj);
		}
		else if (new_json_type == cbor::COT_ARRAY)
		{
			const auto& new_json_str = cbor_encode(new_val);
			const auto& diff_json_str = cbor_encode(diff_json);
			const auto& new_json_array = new_val->as_array();
			const auto& diff_json_array = diff_json.as_array();
			cbor::CborArrayValue result_array = result->as_array();
			for (size_t i = 0; i < diff_json_array.size(); i++)
			{
				if (!diff_json_array[i]->is_array())
					throw CborDiffException("diffjson format error for array diff");
				auto diff_item = diff_json_array[i]->as_array();
				if (diff_item.size() != 3)
					throw CborDiffException("diffjson format error for array diff");
				auto op_item = diff_item[0]->as_string();
				auto pos = diff_item[1]->force_as_int(); // pos是old的pos， FIXME： 新旧对象的pos不一定一样
				auto inner_diff_json = diff_item[2];
				// FIXME； 一个array有多项变化的时候， diff里的索引是用原始对象的index，所以这里应该找出 pos => old_json中同值的pos
				if (op_item == std::string("-"))
				{
					// 删除元素，需要回滚
					result_array.insert(result_array.begin() + pos, inner_diff_json);
				}
				else if (op_item == std::string("+"))
				{
					// 添加元素，需要回滚
					result_array.erase(result_array.begin() + pos);
				}
				else if (op_item == std::string("~"))
				{
					// 修改元素
					auto sub_item_new = rollback(new_json_array[i], std::make_shared<DiffResult>(*inner_diff_json));
					result_array[pos] = sub_item_new;
				}
				else
				{
					throw CborDiffException(std::string("not supported diff array op now: ") + op_item);
				}
			}
			return cbor::CborObject::create_array(result_array);
		}
		else
		{
			throw CborDiffException(std::string("not supported json value type to rollback diff from ") + cbor_to_hex(new_val));
		}
		return result;
	}

}
