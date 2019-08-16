#pragma once
#ifndef json_reader_h
#define json_reader_h

#include <stack>
#include <uvm/lprefix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <cassert>

#include <uvm/lua.h>

#include <uvm/lauxlib.h>
#include <uvm/lualib.h>
#include <uvm/uvm_tokenparser.h>
#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>
#include <uvm/uvm_lutil.h>

typedef const char* Location;
#define StackLimit 100

enum TokenType {
	tokenEndOfStream = 0,
	tokenObjectBegin,
	tokenObjectEnd,
	tokenArrayBegin,
	tokenArrayEnd,
	tokenString,
	tokenNumber,
	tokenTrue,
	tokenFalse,
	tokenNil,
	//tokenNaN,
	//tokenPosInf,
	//tokenNegInf,
	tokenArraySeparator,
	tokenMemberSeparator,
	//tokenComment,
	tokenError
};

class Token {
public:
	TokenType type_;
	Location start_;
	Location end_;
};

class ErrorInfo {
public:
	Token token_;
	std::string message_;
	Location extra_;
};

typedef std::deque<ErrorInfo> Errors;

class Json_Reader {
private:
	typedef char Char;
	lua_State *L_;
	std::string source_;
	typedef std::stack<UvmStorageValue> Nodes;
	Nodes nodes_;
	
	//JSONCPP_STRING document_;
	Location begin_;
	Location end_;
	Location current_;
	//Location lastValueEnd_;
	//UvmStorageValue* lastValue_;
	//JSONCPP_STRING commentsBefore_;
	//Features features_;
	//bool collectComments_;
	bool readValue();
	bool readToken(Token& token);
	void skipSpaces();
	Char getNextChar();
	bool readString();
	UvmStorageValue* currentValue();
	bool decodeString(Token& token, bool is_skip_quote, std::string& decoded);
	bool addError(const std::string& message,
		Token& token,
		Location extra);
	bool readObject(Token& token);
	bool readKeyToken(Token& token);
	bool readArray(Token& token);
	bool readNumber(bool checkInf);
	bool decodeNumber(Token& token,UvmStorageValue* number);
	bool decodeDouble(Token& token, UvmStorageValue* doubleval);
	bool decodeInteger(Token& token, UvmStorageValue* intVal);
	bool match(Location pattern, int patternLength);

public:
	ErrorInfo error;
	//Errors errors_;
	bool parse(lua_State *L, std::string& str, UvmStorageValue* root);
};

#endif
