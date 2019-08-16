#define json_reader_cpp

#include <uvm/json_reader.h>

using namespace uvm::blockchain;

bool Json_Reader::addError(const std::string& message,
	Token& token,
	Location extra) {
	error.token_ = token;
	error.message_ = message;
	error.extra_ = extra;
	return false;
}

Json_Reader::Char Json_Reader::getNextChar() {
	if (current_ == end_)
		return 0;
	return *current_++;
}
void Json_Reader::skipSpaces() {
	while (current_ != end_) {
		Char c = *current_;
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			++current_;
		else
			break;
	}
}

bool Json_Reader::match(Location pattern, int patternLength) {
	if (end_ - current_ < patternLength)
		return false;
	int index = patternLength;
	while (index--)
		if (current_[index] != pattern[index])
			return false;
	current_ += patternLength;
	return true;
}

bool Json_Reader::readKeyToken(Token& token) {
	skipSpaces();
	token.start_ = current_;
	Char c = getNextChar();
	bool ok = false;
	std::string decodestr;
	//bool is_skip_quote;
	switch (c) {
	case '{':
	case '}':
	case '[':
	case ']':
	case ',':
	case ':':
	case 0:
		return addError("invalid key.", token, current_);
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return addError("not allowed digit begin key.", token, current_);
		break;
	case '"':
		token.type_ = tokenString;
		ok = readString();
		//is_skip_quote = true;
		break;
	default:
		//try read string  if pre is { or ,
		/*
		Char c = 0;
		ok = false;
		while (current_ != end_) {
			c = getNextChar();
			if (c == '\\')
				getNextChar();
			else if (c == ':') {
				ok = true;
				current_--;
				token.type_ = tokenString;
				break;
			}
		}
		*/
		break;
	}
	if (!ok)
		token.type_ = tokenError;
	token.end_ = current_;
	return true;
}
bool Json_Reader::readToken(Token& token) {
	skipSpaces();
	token.start_ = current_;
	Char c = getNextChar();
	bool ok = true;
	switch (c) {
	case '{':
		token.type_ = tokenObjectBegin;
		break;
	case '}':
		token.type_ = tokenObjectEnd;
		break;
	case '[':
		token.type_ = tokenArrayBegin;
		break;
	case ']':
		token.type_ = tokenArrayEnd;
		break;
	case '"':
		token.type_ = tokenString;
		ok = readString();
		break;
	/*case '/':
		token.type_ = tokenComment;
		ok = readComment();
		break;
		*/
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	/*case '-':*/
		token.type_ = tokenNumber;
		readNumber(false);
		break; 
	case 't':
		token.type_ = tokenTrue;
		ok = match("rue", 3);
		break;
	case 'f':
		token.type_ = tokenFalse;
		ok = match("alse", 4);
		break;
	case 'n':
		token.type_ = tokenNil;
		ok = match("il", 2);
		break;
	case ',':
		token.type_ = tokenArraySeparator;
		break;
	case ':':
		token.type_ = tokenMemberSeparator;
		break;
	case 0:
		token.type_ = tokenEndOfStream;
		break;
	default:
		ok = false;
		break;
	}
	if (!ok)
		token.type_ = tokenError;
	token.end_ = current_;
	return ok;
}
bool Json_Reader::readString() {
	Char c = 0;
	while (current_ != end_) {
		c = getNextChar();
		if (c == '\\')
			getNextChar();
		else if (c == '"')
			break;
	}
	return c == '"';
}

bool Json_Reader::readNumber(bool checkInf) {
	const char* p = current_;
	if (checkInf && p != end_ && *p == 'I') {
		current_ = ++p;
		return false;
	}
	char c = '0'; // stopgap for already consumed character
				  // integral part
	while (c >= '0' && c <= '9')
		c = (current_ = p) < end_ ? *p++ : '\0';
	// fractional part
	if (c == '.') {
		c = (current_ = p) < end_ ? *p++ : '\0';
		while (c >= '0' && c <= '9')
			c = (current_ = p) < end_ ? *p++ : '\0';
	}
	// exponential part
	if (c == 'e' || c == 'E') {
		c = (current_ = p) < end_ ? *p++ : '\0';
		if (c == '+' || c == '-')
			c = (current_ = p) < end_ ? *p++ : '\0';
		while (c >= '0' && c <= '9')
			c = (current_ = p) < end_ ? *p++ : '\0';
	}
	return true;
}

bool Json_Reader::parse(lua_State *L, std::string& str,UvmStorageValue* root) {
	while(!nodes_.empty()) {
		nodes_.pop();
	}
	if (!L || !root) {
		return false;
	}
	error.message_ = "";

	L_ = L;
	UvmStorageValue value;
	nodes_.push(value);
	//UvmStorageValue* pval = reinterpret_cast<UvmStorageValue*>(uvm::lua::lib::malloc_managed_string(L_, sizeof(UvmStorageValue),nullptr));
	//nodes_.push(*pval);
	Token token;
	source_ = str;
	begin_ = str.c_str();
	end_ = str.length() + begin_;
	current_ = begin_;
	bool ok = readValue();

	if(ok){
		if (nodes_.size() == 1) {
			if (current_ < end_) {
				ok = false;
				addError("extra data after json object", token, current_);
			}
			else {
				*root = nodes_.top();  //parse scuccess
			}
		}
		else {
			ok = false;
			addError("internal error(stack not balance)", token, current_);
		}
	}

	while(!nodes_.empty()) {
		nodes_.pop();
	}
	return ok;
}
UvmStorageValue* Json_Reader::currentValue() { return (&(nodes_.top())); }

bool Json_Reader::decodeString(Token& token,bool is_skip_quote,std::string& decoded) {
	Location current;
	Location end;
	if (is_skip_quote) {
		current = token.start_ + 1; // skip '"'
		end = token.end_ - 1;       // do not include '"'
	}
	else {
		current = token.start_ ; 
		end = token.end_ ; 
	}
	//std::string decoded;
	
	while (current != end) {
		Char c = *current++;
		if (c == '"')
			break;
		else if (c == '\n' || c == '\r') {
			return addError("unfinished string", token, current);
		}
		else if (c == '\\') {
			if (current == end)
				return addError("Empty escape sequence in string", token, current);
			Char escape = *current++;
			switch (escape) {
			case '"':
				decoded += '"';
				break;
			case '/':
				decoded += '/';
				break;
			case '\\':
				decoded += '\\';
				break;
			case 'b':
				decoded += '\b';
				break;
			case 'f':
				decoded += '\f';
				break;
			case 'n':
				decoded += '\n';
				break;
			case 'r':
				decoded += '\r';
				break;
			case 't':
				decoded += '\t';
				break;
			case 'u': {
				/*unsigned int unicode;
				if (!decodeUnicodeCodePoint(token, current, end, unicode))
					return false;
				decoded += codePointToUTF8(unicode);*/
			} break;
			default:
				return addError("Bad escape sequence in string", token, current);
			}
		}
		else {
			decoded += c;
		}
	}
	
	return true;
}

//exist node
bool Json_Reader::readObject(Token& token) {
	Token tokenName;
	std::string name;
	std::string errormsg;

	auto obj = currentValue();
	obj->type = StorageValueTypes::storage_value_unknown_table;
	obj->value.table_value = uvm::lua::lib::create_managed_lua_table_map(L_);
	auto ptab = obj->value.table_value;
	//UvmStorageValue obj;
	//obj.type = StorageValueTypes::storage_value_unknown_table;
	//nodes_.push(obj);
	if (current_ != end_ && *current_ == '}') // empty map
	{
		Token endMap;
		readToken(endMap);
		return true;
	}
	while (readKeyToken(tokenName)) {
		/*bool initialTokenOk = true;
		while (tokenName.type_ == tokenComment && initialTokenOk)
			initialTokenOk = readToken(tokenName);
		if (!initialTokenOk)
			break;
		if (tokenName.type_ == tokenObjectEnd && name.empty()) // empty object
			return true;
		*/
		name.clear();
		if (tokenName.type_ == tokenString) {
			bool is_skip_quote = false;
			if (*tokenName.start_ == '"') {
				is_skip_quote = true;
			}
			if (!decodeString(tokenName, is_skip_quote, name)) {
				return addError("invalid object member name.", tokenName, current_);
			}
		}
		/*else if (tokenName.type_ == tokenNumber && features_.allowNumericKeys_) {
			Value numberName;
			if (!decodeNumber(tokenName, numberName))
				return recoverFromError(tokenObjectEnd);
			name = JSONCPP_STRING(numberName.asCString());
		}*/
		else {
			break;
		}

		Token colon;
		if (!readToken(colon) || colon.type_ != tokenMemberSeparator) {
			std::string errormsg = "Missing ':' after object member name";
			if (colon.start_ < end_ && colon.start_ >= begin_) {
				errormsg = errormsg + ", but got '" + *(colon.start_) + "'";
			}
			return addError(errormsg, colon, current_);

			//return addError("Missing ':' after object member name", colon,current_);
		}
		UvmStorageValue value;
		nodes_.push(value);
		//UvmStorageValue* pval = reinterpret_cast<UvmStorageValue*>(uvm::lua::lib::malloc_managed_string(L_, sizeof(UvmStorageValue), nullptr));
		//nodes_.push(*pval);
		bool ok = readValue(); //recursion
		if (!ok) {
			nodes_.pop();
			return false;
		}// error already set
		ptab->erase(name);
		ptab->insert(std::map<std::string, UvmStorageValue>::value_type(name, *currentValue()));
		//ptab->[name] = *currentValue();
		nodes_.pop();
		
		Token comma;
		if (!readToken(comma) ||
			(comma.type_ != tokenObjectEnd && comma.type_ != tokenArraySeparator /*&&
				comma.type_ != tokenComment*/)) {
			errormsg = "Missing ',' or '}' in object declaration";
			if (comma.start_ < end_ && comma.start_ >= begin_) {
				errormsg = errormsg + ", but got '" + *(comma.end_) + "'";
			}
			return addError(errormsg, comma, current_);
			//return addError("Missing ',' or '}' in object declaration",comma, current_);
		}
		/*bool finalizeTokenOk = true;
		while (comma.type_ == tokenComment && finalizeTokenOk)
			finalizeTokenOk = readToken(comma);*/
		if (comma.type_ == tokenObjectEnd)
			return true;
	}
	errormsg = "Missing '}' or object member name";
	if (tokenName.end_ < end_ && tokenName.start_ >= begin_) {
		errormsg = errormsg + ", but got '" + *(tokenName.start_) + "'";
	}
	return addError(errormsg, tokenName, current_);

	//return addError("Missing '}' or object member name", tokenName,current_);
}

bool Json_Reader::readArray(Token& token) {
	skipSpaces();
	auto arr = currentValue();
	arr->type = StorageValueTypes::storage_value_unknown_array;
	arr->value.table_value = uvm::lua::lib::create_managed_lua_table_map(L_);
	auto parr = arr->value.table_value;

	if (current_ != end_ && *current_ == ']') // empty array
	{
		Token endArray;
		readToken(endArray);
		return true;
	}

	int index = 0;
	for (;;) {
		UvmStorageValue value;
		nodes_.push(value);
		bool ok = readValue();
		if (!ok) { // error already set
			nodes_.pop();
			return false;
		}
		std::string idx = std::to_string(++index);  //begin from 1
		parr->erase(idx);
		//parr->insert(std::map<std::string, UvmStorageValue>::value_type(idx, *currentValue()));
		(*parr)[idx] = *currentValue();
		nodes_.pop();
		////////////////

		Token currentToken;
		// Accept Comment after last item in the array.
		ok = readToken(currentToken);
		/*while (currentToken.type_ == tokenComment && ok) {
			ok = readToken(currentToken);
		}*/
		bool badTokenType = (currentToken.type_ != tokenArraySeparator &&
			currentToken.type_ != tokenArrayEnd);
		if (!ok || badTokenType) {
			std::string errormsg = "Missing ',' or ']' in array declaration";
			if (currentToken.start_ < end_ && currentToken.start_ >= begin_) {
				errormsg = errormsg + ", but got '" + *(currentToken.start_) + "'";
			}
			return addError(errormsg,currentToken, current_);
		}
		if (currentToken.type_ == tokenArrayEnd)
			break;
	}
	return true;
}

bool Json_Reader::decodeNumber(Token& token, UvmStorageValue* number) {
	// Attempts to parse the number as an integer. If the number is
	// larger than the maximum supported value of an integer then
	// we decode the number as a double.
	Location current = token.start_;
	bool isNegative = *current == '-';
	bool isDouble = false;
	if (isNegative)
		++current;

	while (current < token.end_) {
		Char c = *current++;
		if (c == '.') {
			isDouble = true;
			break;
		}
	}

	if (isDouble) {
		return decodeDouble(token, number);
	}
	else {
		return decodeInteger(token, number);
	}

	


	/*
	// TODO: Help the compiler do the div and mod at compile time or get rid of
	// them.
	Value::LargestUInt maxIntegerValue =
		isNegative ? Value::LargestUInt(Value::maxLargestInt) + 1
		: Value::maxLargestUInt;
	Value::LargestUInt threshold = maxIntegerValue / 10;
	Value::LargestUInt value = 0;
	while (current < token.end_) {
		Char c = *current++;
		if (c < '0' || c > '9')
			return decodeDouble(token, number);
		Value::UInt digit(static_cast<Value::UInt>(c - '0'));
		if (value >= threshold) {
			// We've hit or exceeded the max value divided by 10 (rounded down). If
			// a) we've only just touched the limit, b) this is the last digit, and
			// c) it's small enough to fit in that rounding delta, we're okay.
			// Otherwise treat this number as a double to avoid overflow.
			if (value > threshold || current != token.end_ ||
				digit > maxIntegerValue % 10) {
				return decodeDouble(token, decoded);
			}
		}
		value = value * 10 + digit;
	}
	if (isNegative && value == maxIntegerValue)
		decoded = Value::minLargestInt;
	else if (isNegative)
		decoded = -Value::LargestInt(value);
	else if (value <= Value::LargestUInt(Value::maxInt))
		decoded = Value::LargestInt(value);
	else
		decoded = value;
	
	return true;
	*/
}

bool Json_Reader::decodeInteger(Token& token, UvmStorageValue* intVal) {
	lua_Integer value = 0;
	std::string buffer(token.start_, token.end_);
	std::istringstream is(buffer);
	if (!(is >> value))
		return addError("'" + std::string(token.start_, token.end_) +
			"' is not a integer.", token, current_);
	intVal->type = StorageValueTypes::storage_value_int;
	intVal->value.int_value = value;
	return true;
}

bool Json_Reader::decodeDouble(Token& token, UvmStorageValue* doubleVal) {
	lua_Number value = 0;
	std::string buffer(token.start_, token.end_);
	std::istringstream is(buffer);
	if (!(is >> value))
		return addError("'" + std::string(token.start_, token.end_) +
			"' is not a number.", token, current_);
	doubleVal->type = StorageValueTypes::storage_value_number;
	doubleVal->value.number_value = value;
	return true;
}

//exist node
bool Json_Reader::readValue() {
	Token token;
	std::string decodestr;
	UvmStorageValue number;
	auto value = currentValue();
	//  To preserve the old behaviour we cast size_t to int.
	if (static_cast<int>(nodes_.size()) > StackLimit)
		return addError("Exceeded stackLimit in readValue().", token, nullptr);

	if (!readToken(token)) {
		std::string errormsg = "unknown token ";
		if (token.start_ < end_ && token.start_ >= begin_) {
			errormsg = errormsg + "'" + *(token.start_) + "'";
		}
		return addError(errormsg, token, current_);
	}
	//skipCommentTokens(token);
	bool successful = true;

	switch (token.type_) {
	
	case tokenObjectBegin:
		successful = readObject(token);
		break;
	case tokenArrayBegin:
		successful = readArray(token);
		break;
	case tokenNumber:
		successful = decodeNumber(token,&number);
		if (successful) {
			auto value = currentValue();
			*value = number;
		}
		break;
	case tokenString:
		successful = decodeString(token,true, decodestr);
		if (successful) {
			value->type = StorageValueTypes::storage_value_string;
			value->value.string_value = uvm::lua::lib::malloc_managed_string(L_, decodestr.length() + 1);
			memset(value->value.string_value, 0x0, decodestr.length() + 1);
			strncpy(value->value.string_value, decodestr.c_str(), decodestr.length());
			assert(nullptr != value->value.string_value);
		}
		break;
	case tokenTrue: {
		value->type = StorageValueTypes::storage_value_bool;
		value->value.bool_value = true;
	} break;
	case tokenFalse: {
		value->type = StorageValueTypes::storage_value_bool;
		value->value.bool_value = false;
	} break;
	case tokenNil: {
		value->type = StorageValueTypes::storage_value_null;
	} break;
	/*
	case tokenNaN: {
		Value v(std::numeric_limits<double>::quiet_NaN());
		currentValue().swapPayload(v);
		currentValue().setOffsetStart(token.start_ - begin_);
		currentValue().setOffsetLimit(token.end_ - begin_);
	} break;
	case tokenPosInf: {
		Value v(std::numeric_limits<double>::infinity());
		currentValue().swapPayload(v);
		currentValue().setOffsetStart(token.start_ - begin_);
		currentValue().setOffsetLimit(token.end_ - begin_);
	} break;
	case tokenNegInf: {
		Value v(-std::numeric_limits<double>::infinity());
		currentValue().swapPayload(v);
		currentValue().setOffsetStart(token.start_ - begin_);
		currentValue().setOffsetLimit(token.end_ - begin_);
	} break;
	case tokenArraySeparator:
	case tokenObjectEnd:
	case tokenArrayEnd:
		if (features_.allowDroppedNullPlaceholders_) {
			// "Un-read" the current token and mark the current value as a null
			// token.
			current_--;
			Value v;
			currentValue().swapPayload(v);
			currentValue().setOffsetStart(current_ - begin_ - 1);
			currentValue().setOffsetLimit(current_ - begin_);
			break;
		} // else, fall through ...
		*/
	default:
		currentValue()->type = StorageValueTypes::storage_value_not_support;
		return addError("Syntax error: value, object or array expected.", token,nullptr);
	}
	/*
	if (collectComments_) {
		lastValueEnd_ = current_;
		lastValue_ = &currentValue();
	}
	*/
	//nodes_.pop();
	return successful;
}
