#include <jsondiff/helper.h>
#include <boost/algorithm/string/predicate.hpp>

namespace jsondiff
{
	namespace utils
	{
		bool string_ends_with(std::string str, std::string end)
		{
			auto pos = str.find(end);
			return pos >= 0 && (pos + end.size() == str.size());
		}

		std::string string_without_ext(std::string str, std::string ext)
		{
			if (!string_ends_with(str, ext))
				return str;
			return str.substr(0, str.size() - ext.size());
		}
	}
}
