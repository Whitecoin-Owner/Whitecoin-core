#pragma once

#include <uvm/lprefix.h>
#include <boost/asio/buffer.hpp>
#include <boost/variant.hpp>
#include <fc/variant.hpp>

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <list>
#include <vector>
#include <chrono>
#include <algorithm>
#include <memory>
#include <cborcpp/cbor.h>

struct lua_State;

namespace uvm
{
	namespace util
	{
		char file_separator();

		char *file_separator_str();

		char *copy_str(const char *str);

		bool ends_with(const std::string &str, const std::string &suffix);

		bool starts_with(const std::string &str, const std::string &prefix);

		bool compare_string_list(std::list<std::string> &l1, std::list<std::string> &l2);
		bool compare_key(const std::string& first, const std::string& second);

		void replace_all(std::string& str, const std::string& from, const std::string& to);

		/**
		* escape string to use in json
		*/
		std::string escape_string(std::string &str);

		/**
		* unescape string used in json
		*/
		std::string unescape_string(std::string &str);

		std::vector<std::string> &string_split(const std::string &s, char delim, std::vector<std::string> &elems);

		std::vector<std::string> string_split(const std::string &s, char delim);

		std::string string_trim(std::string source);

		template <typename IterT>
		std::string string_join(IterT begin, IterT end, std::string sep)
		{
			std::stringstream ss;
			bool is_first = true;
			auto i = begin;
			while(i!=end)
			{
				if (!is_first)
					ss << sep;
				is_first = false;
				ss << *i;
				++i;
			}
			return ss.str();
		}

		template <typename T>
		bool vector_contains(std::vector<T> &col, const T &item)
		{
			return std::find(col.begin(), col.end(), item) != col.end();
		}

		template <typename ColType, typename ColType2>
		void append_range(ColType &col, ColType2 &items)
		{
			for(const auto &item : items)
			{
				col.push_back(item);
			}
		}

		template<typename TEMP,typename T>
        bool find(TEMP begin,TEMP end,T to_find)
        {
            TEMP in=begin;
            while (in!=end)
            {
                if(*in==to_find)
                {
                    return true;
                }
                in++;
            }
            return false;
        }

		size_t string_lines_count(const std::string &str);

        class TimeDiff
        {
        private:
          std::chrono::time_point<std::chrono::system_clock> _start_time;
          std::chrono::time_point<std::chrono::system_clock> _end_time;
        public:
          inline TimeDiff(bool start_now=true)
          {
            if (start_now)
              start();
          }
          inline void start()
          {
            _start_time = std::chrono::system_clock::now();
          }
          inline void end()
          {
            _end_time = std::chrono::system_clock::now();
          }
          inline std::chrono::duration<double> diff_time(bool end_time=true)
          {
            if (end_time)
              end();
            return _end_time - _start_time;
          }
          inline double diff_timestamp(bool end_time=true)
          {
            return diff_time(end_time).count();
          }
        };
		
		typedef std::vector<unsigned char> Buffer;
		struct BufferSlice {
			std::shared_ptr<Buffer> source;
			size_t offset;

			size_t size() const;
			unsigned char at(size_t pos) const;
			bool empty() const;
			BufferSlice slice(size_t from) const;
			std::shared_ptr<Buffer> clone_slice(size_t from, size_t length) const;
			std::shared_ptr<Buffer> to_buffer() const;

			static BufferSlice create(std::shared_ptr<Buffer> source, size_t offset = 0);
		};
		std::string buffer_to_hex(const Buffer& buffer);
		int64_t hex_to_int(const std::string& hex_str);

		enum RlpObjectType {
			ROT_BYTES = 0,
			ROT_LIST = 1
		};

		template <typename T>
		Buffer int_little_endian_pack(T v) {
			auto val = v;
			size_t cycle_count = sizeof(v);
			Buffer result(cycle_count);
			for (size_t i = 0; i < cycle_count; i++) {
				uint8_t b = uint8_t(val);
				val >>= 8;
				result[cycle_count - i - 1] = b;
			}
			return result;
		}

		template <typename T>
		T little_endian_unpack_int(const Buffer& buf) {
			T result = 0;
			if (buf.empty())
				return 0;
			for (size_t i = 0; i < buf.size();i++) {
				unsigned char c = buf[i];
				result = (result << 8) + c;
			}
			return result;
		}

		struct RlpObject;
		struct RlpObject {
			RlpObjectType type;
			std::vector<unsigned char> bytes;
			std::vector<std::shared_ptr<RlpObject> > objects;

			template <typename T>
			static std::shared_ptr<RlpObject> pack_to_bytes(const T& value) {
				auto obj = std::make_shared<RlpObject>();
				const auto& packed = int_little_endian_pack(value);
				Buffer bytes(packed.size());
				memcpy(bytes.data(), packed.data(), packed.size());
				obj->type = RlpObjectType::ROT_BYTES;
				obj->bytes = bytes;
				return obj;
			}

			static std::shared_ptr<RlpObject> from_char(char c);
			static std::shared_ptr<RlpObject> from_int32(int32_t value);
			static std::shared_ptr<RlpObject> from_int64(int64_t value);
			static std::shared_ptr<RlpObject> from_string(const std::string& str);
			static std::shared_ptr<RlpObject> from_bytes(const Buffer& buf);
			static std::shared_ptr<RlpObject> from_list(std::vector<std::shared_ptr<RlpObject> > objects);

			int32_t to_int32() const;
			int64_t to_int64() const;
			char to_char() const;
			std::string to_string() const;
			Buffer to_buffer() const;

		};

		class RlpEncoder {
		public:
			Buffer encode(unsigned char input);
			Buffer encode(const std::string& input);
			Buffer encode(const std::vector<unsigned char>& input);
			Buffer encode(const std::vector<std::shared_ptr<RlpObject> >& objects);
			Buffer encode(std::shared_ptr<RlpObject> obj);
			Buffer encode_length(uint32_t length, int32_t offset);
		};

		struct DecodeResult {
			std::shared_ptr<RlpObject> data;
			BufferSlice source;
		};

		class RlpDecoder {
		public:
			std::shared_ptr<RlpObject> decode(const Buffer& bytes);
		private:
			DecodeResult decode_non_empty(const BufferSlice& bytes);
		};
		
		class stringbuffer {
		private:
			std::vector<std::string> _parts;
		public:
			stringbuffer* put(const std::string& value);
			stringbuffer* put(int32_t value);
			stringbuffer* put(int64_t value);
			stringbuffer* put(uint32_t value);
			stringbuffer* put(uint64_t value);
			stringbuffer* put(double value);
			stringbuffer* put_bool(bool value);
			void clear();
			std::string str() const;
		};

		std::string unhex(const std::string& int_str);

		std::string hex(const std::string& hex_str);

		std::string convert_pre(int old_base, int new_base, std::string source_str);

		std::string json_ordered_dumps(const fc::variant& value);
		cbor::CborObjectP nested_cbor_object_to_array(const cbor::CborObject* cbor_value);

	} // end namespace uvm::util
} // end namespace uvm
