#pragma once
#include <exception>
#include <stdint.h>
#include <string>
#include <memory>

namespace cbor_diff {

	class CborDiffException : public std::exception
	{
	protected:
		std::string _name_value;
		std::string _error_msg;
	public:
		inline CborDiffException() { }
		inline CborDiffException(const std::string &name_value, const std::string &error_msg)
			: _name_value(name_value), _error_msg(error_msg) {}
		inline CborDiffException(const CborDiffException& other) {
			_name_value = other._name_value;
			_error_msg = other._error_msg;
		}
		inline CborDiffException(const char *msg)
		{
			_error_msg = msg;
		}
		inline CborDiffException(const std::string &msg)
		{
			_error_msg = msg;
		}
		inline CborDiffException& operator=(const CborDiffException& other) {
			if (this != &other)
			{
				_error_msg = other._error_msg;
			}
			return *this;
		}
		inline virtual ~CborDiffException() {}

#ifdef _WIN32
		inline virtual const char* what() const
#else
		inline virtual const char* what() const noexcept
#endif 
		{
			return _error_msg.c_str();
		}
		inline std::string name() const
		{
			return _name_value;
		}
		inline virtual std::shared_ptr<cbor_diff::CborDiffException> dynamic_copy_exception()const
		{
			return std::make_shared<CborDiffException>(*this);
		}
		inline virtual void dynamic_rethrow_exception()const
		{
			throw this;
		}
	};
}
