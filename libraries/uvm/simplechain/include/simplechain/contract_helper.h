#pragma once
#include <simplechain/config.h>
#include <streambuf>
#include <istream>
#include <ostream>

namespace simplechain {

	class LikeFile {
	public:
		virtual size_t common_read(void* buf, size_t element_size, size_t count) = 0;
		virtual int common_close() = 0;
		virtual long common_tell() = 0;
	};

	class FileWrapper : public LikeFile {
	private:
		FILE* fp;
	public:
		FileWrapper(FILE* _fp) : fp(_fp) {}

		virtual size_t common_read(void* buf, size_t element_size, size_t count);
		virtual int common_close();
		virtual long common_tell();
	};

	class StreamFileWrapper : public LikeFile {
	private:
		std::istream* stream;
	public:
		StreamFileWrapper(std::istream* _stream) : stream(_stream) {}

		virtual size_t common_read(void* buf, size_t element_size, size_t count);
		virtual int common_close();
		virtual long common_tell();
	};

	class ContractHelper
	{
	public:

		static int common_fread_int(LikeFile* fp, int* dst_int);
		static int common_fwrite_int(FILE* fp, const int* src_int);
		static int common_fwrite_stream(FILE* fp, const void* src_stream, int len);
		static int common_fread_octets(LikeFile* fp, void* dst_stream, int len);
		static std::string to_printable_hex(unsigned char chr);
		static int save_code_to_file(const std::string& name, UvmModuleByteStream *stream, char* err_msg);
		static uvm::blockchain::Code load_contract_from_file(const fc::path &path);
		static uvm::blockchain::Code load_contract_from_common_stream_and_close(LikeFile* fp, int fsize);
	};
}
