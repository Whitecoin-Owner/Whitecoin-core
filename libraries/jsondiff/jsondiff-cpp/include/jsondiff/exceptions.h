#ifndef JSONDIFF_EXCEPTIONS_H
#define JSONDIFF_EXCEPTIONS_H


#include <exception>
#include <stdint.h>
#include <string>
#include <memory>

namespace jsondiff
{
	class JsonDiffException : public std::exception
	{
	protected:
		std::string _name_value;
		std::string _error_msg;
	public:
		inline JsonDiffException() { }
		inline JsonDiffException(const std::string &name_value, const std::string &error_msg)
			: _name_value(name_value), _error_msg(error_msg) {}
		inline JsonDiffException(const JsonDiffException& other) {
			_name_value = other._name_value;
			_error_msg = other._error_msg;
		}
		inline JsonDiffException(const char *msg)
		{
			_error_msg = msg;
		}
		inline JsonDiffException(const std::string &msg)
		{
			_error_msg = msg;
		}
		inline JsonDiffException& operator=(const JsonDiffException& other) {
			if (this != &other)
			{
				_error_msg = other._error_msg;
			}
			return *this;
		}
		inline virtual ~JsonDiffException() {}

#ifdef WIN32
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
		inline virtual std::shared_ptr<jsondiff::JsonDiffException> dynamic_copy_exception()const
		{
			return std::make_shared<JsonDiffException>(*this);
		}
		inline virtual void dynamic_rethrow_exception()const
		{
			jsondiff::JsonDiffException::dynamic_rethrow_exception();
		}
	};
}

#endif
