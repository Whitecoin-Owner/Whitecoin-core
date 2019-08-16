#include <simplechain/contract_helper.h>
#include <simplechain/contract_entry.h>
#include <boost/uuid/sha1.hpp>
#include <exception>
#include <uvm/uvm_api.h>

namespace simplechain {
	using namespace std;

	size_t FileWrapper::common_read(void* buf, size_t element_size, size_t count) {
		return fread(buf, element_size, count, fp);
	}

	size_t StreamFileWrapper::common_read(void* buf, size_t element_size, size_t count) {
		return stream->readsome((char*)buf, element_size*count);
	}

	int FileWrapper::common_close() {
		return fclose(fp);
	}

	int StreamFileWrapper::common_close() {
		return 1;
	}

	long FileWrapper::common_tell() {
		return ftell(fp);
	}

	long StreamFileWrapper::common_tell() {
		return (long)stream->tellg();
	}

	int ContractHelper::common_fread_int(LikeFile* fp, int* dst_int)
	{
		int ret;
		unsigned char uc4, uc3, uc2, uc1;

		ret = (int)fp->common_read(&uc4, sizeof(unsigned char), 1);
		if (ret != 1)
			return ret;
		ret = (int)fp->common_read(&uc3, sizeof(unsigned char), 1);
		if (ret != 1)
			return ret;
		ret = (int)fp->common_read(&uc2, sizeof(unsigned char), 1);
		if (ret != 1)
			return ret;
		ret = (int)fp->common_read(&uc1, sizeof(unsigned char), 1);
		if (ret != 1)
			return ret;

		*dst_int = (uc4 << 24) + (uc3 << 16) + (uc2 << 8) + uc1;

		return 1;
	}

	int ContractHelper::common_fwrite_int(FILE* fp, const int* src_int)
	{
		int ret;
		unsigned char uc4, uc3, uc2, uc1;
		uc4 = ((*src_int) & 0xFF000000) >> 24;
		uc3 = ((*src_int) & 0x00FF0000) >> 16;
		uc2 = ((*src_int) & 0x0000FF00) >> 8;
		uc1 = (*src_int) & 0x000000FF;

		ret = (int)fwrite(&uc4, sizeof(unsigned char), 1, fp);
		if (ret != 1)
			return ret;
		ret = (int)fwrite(&uc3, sizeof(unsigned char), 1, fp);
		if (ret != 1)
			return ret;
		ret = (int)fwrite(&uc2, sizeof(unsigned char), 1, fp);
		if (ret != 1)
			return ret;
		ret = (int)fwrite(&uc1, sizeof(unsigned char), 1, fp);
		if (ret != 1)
			return ret;

		return 1;
	}

	int ContractHelper::common_fwrite_stream(FILE* fp, const void* src_stream, int len)
	{
		return (int)fwrite(src_stream, len, 1, fp);
	}

	int ContractHelper::common_fread_octets(LikeFile* fp, void* dst_stream, int len)
	{
		return (int)fp->common_read(dst_stream, len, 1);
	}


#define PRINTABLE_CHAR(chr) \
if (chr >= 0 && chr <= 9)  \
    chr = chr + '0'; \
else \
    chr = chr + 'a' - 10; 

	std::string ContractHelper::to_printable_hex(unsigned char chr)
	{
		unsigned char high = chr >> 4;
		unsigned char low = chr & 0x0F;
		char tmp[16];

		PRINTABLE_CHAR(high);
		PRINTABLE_CHAR(low);

		snprintf(tmp, sizeof(tmp), "%c%c", high, low);
		return string(tmp);
	}

	int ContractHelper::save_code_to_file(const string& name, UvmModuleByteStream *stream, char* err_msg)
	{
		boost::uuids::detail::sha1 sha;
		unsigned int digest[5];

		UvmModuleByteStream* p_new_stream = new UvmModuleByteStream();
		if (NULL == p_new_stream)
		{
			strcpy(err_msg, "malloc UvmModuleByteStream fail");
			return -1;
		}
		p_new_stream->is_bytes = stream->is_bytes;
		p_new_stream->buff = stream->buff;
		for (int i = 0; i < stream->contract_apis.size(); ++i)
		{
			int new_flag = 1;
			for (int j = 0; j < stream->offline_apis.size(); ++j)
			{
				if (stream->contract_apis[i] == stream->offline_apis[j])
				{
					new_flag = 0;
					continue;
				}
			}

			if (new_flag)
			{
				p_new_stream->contract_apis.push_back(stream->contract_apis[i]);
			}
		}
		p_new_stream->offline_apis = stream->offline_apis;
		p_new_stream->contract_emit_events = stream->contract_emit_events;
		p_new_stream->contract_storage_properties = stream->contract_storage_properties;

		p_new_stream->contract_id = stream->contract_id;
		p_new_stream->contract_name = stream->contract_name;
		p_new_stream->contract_level = stream->contract_level;
		p_new_stream->contract_state = stream->contract_state;

		FILE *f = fopen(name.c_str(), "wb");
		if (NULL == f)
		{
			delete (p_new_stream);
			strcpy(err_msg, strerror(errno));
			return -1;
		}

		sha.process_bytes(p_new_stream->buff.data(), p_new_stream->buff.size());
		sha.get_digest(digest);
		for (int i = 0; i < 5; ++i)
			common_fwrite_int(f, (int*)&digest[i]);

		int p_new_stream_buf_size = (int)p_new_stream->buff.size();
		common_fwrite_int(f, &p_new_stream_buf_size);
		p_new_stream->buff.resize(p_new_stream_buf_size);
		common_fwrite_stream(f, p_new_stream->buff.data(), p_new_stream->buff.size());

		int contract_apis_count = (int)p_new_stream->contract_apis.size();
		common_fwrite_int(f, &contract_apis_count);
		for (int i = 0; i < contract_apis_count; ++i)
		{
			int api_len = (int)(p_new_stream->contract_apis[i].length());
			common_fwrite_int(f, &api_len);
			common_fwrite_stream(f, p_new_stream->contract_apis[i].c_str(), api_len);
		}

		int offline_apis_count = (int)p_new_stream->offline_apis.size();
		common_fwrite_int(f, &offline_apis_count);
		for (int i = 0; i < offline_apis_count; ++i)
		{
			int offline_api_len = (int)(p_new_stream->offline_apis[i].length());
			common_fwrite_int(f, &offline_api_len);
			common_fwrite_stream(f, p_new_stream->offline_apis[i].c_str(), offline_api_len);
		}

		int contract_emit_events_count = p_new_stream->contract_emit_events.size();
		common_fwrite_int(f, &contract_emit_events_count);
		for (int i = 0; i < contract_emit_events_count; ++i)
		{
			int event_len = (int)(p_new_stream->contract_emit_events[i].length());
			common_fwrite_int(f, &event_len);
			common_fwrite_stream(f, p_new_stream->contract_emit_events[i].c_str(), event_len);
		}

		int contract_storage_properties_count = p_new_stream->contract_storage_properties.size();
		common_fwrite_int(f, &contract_storage_properties_count);
		for (const auto& storage_info : p_new_stream->contract_storage_properties)
		{
			int storage_len = (int)storage_info.first.length();
			common_fwrite_int(f, &storage_len);
			common_fwrite_stream(f, storage_info.first.c_str(), storage_len);
			int storage_type = storage_info.second;
			common_fwrite_int(f, &storage_type);
		}

		fclose(f);
		delete (p_new_stream);
		return 0;
	}


#define INIT_API_FROM_FILE(dst_set)\
{\
read_count = common_fread_int(f, &api_count); \
if (read_count != 1)\
{\
f->common_close(); \
throw uvm::core::UvmException("Read verify code fail!"); \
}\
for (int i = 0; i < api_count; i++)\
{\
int api_len = 0; \
read_count = common_fread_int(f, &api_len); \
if (read_count != 1)\
{\
f->common_close(); \
throw uvm::core::UvmException("Read verify code fail!"); \
}\
api_buf = (char*)malloc(api_len + 1); \
if (api_buf == NULL) \
{ \
f->common_close(); \
FC_ASSERT(api_buf == NULL, "malloc fail!"); \
}\
read_count = common_fread_octets(f, api_buf, api_len); \
if (read_count != 1)\
{\
f->common_close(); \
free(api_buf); \
throw uvm::core::UvmException("Read verify code fail!"); \
}\
api_buf[api_len] = '\0'; \
dst_set.insert(std::string(api_buf)); \
free(api_buf); \
}\
}

#define INIT_STORAGE_FROM_FILE(dst_map)\
{\
read_count = common_fread_int(f, &storage_count); \
if (read_count != 1)\
{\
f->common_close(); \
throw uvm::core::UvmException("Read verify code fail!"); \
}\
for (int i = 0; i < storage_count; i++)\
{\
int storage_name_len = 0; \
read_count = common_fread_int(f, &storage_name_len); \
if (read_count != 1)\
{\
f->common_close(); \
throw uvm::core::UvmException("Read verify code fail!"); \
}\
storage_buf = (char*)malloc(storage_name_len + 1); \
if (storage_buf == NULL) \
{ \
f->common_close(); \
FC_ASSERT(storage_buf == NULL, "malloc fail!"); \
}\
read_count = common_fread_octets(f, storage_buf, storage_name_len); \
if (read_count != 1)\
{\
f->common_close(); \
free(storage_buf); \
throw uvm::core::UvmException("Read verify code fail!"); \
}\
storage_buf[storage_name_len] = '\0'; \
read_count = common_fread_int(f, (int*)&storage_type); \
if (read_count != 1)\
{\
f->common_close(); \
free(storage_buf); \
throw uvm::core::UvmException("Read verify code fail!"); \
}\
dst_map.insert(std::make_pair(std::string(storage_buf), storage_type)); \
free(storage_buf); \
}\
}

	uvm::blockchain::Code ContractHelper::load_contract_from_common_stream_and_close(LikeFile* f, int fsize) {
		uvm::blockchain::Code code;
		unsigned int digest[5];
		int read_count = 0;
		for (int i = 0; i < 5; ++i)
		{
			read_count = common_fread_int(f, (int*)&digest[i]);
			if (read_count != 1)
			{
				f->common_close();
				throw uvm::core::UvmException("Read verify code fail!");
			}
		}

		int len = 0;
		read_count = common_fread_int(f, &len);
		if (read_count != 1 || len < 0 || (len >= (fsize - f->common_tell())))
		{
			f->common_close();
			throw uvm::core::UvmException("Read verify code fail!");
		}

		code.code.resize(len);
		read_count = common_fread_octets(f, code.code.data(), len);
		if (read_count != 1)
		{
			f->common_close();
			throw uvm::core::UvmException("Read verify code fail!");
		}

		boost::uuids::detail::sha1 sha;
		unsigned int check_digest[5];
		sha.process_bytes(code.code.data(), code.code.size());
		sha.get_digest(check_digest);
		if (memcmp((void*)digest, (void*)check_digest, sizeof(unsigned int) * 5))
		{
			f->common_close();
			throw uvm::core::UvmException("Verify bytescode SHA1 fail!");
		}

		for (int i = 0; i < 5; ++i)
		{
			unsigned char chr1 = (check_digest[i] & 0xFF000000) >> 24;
			unsigned char chr2 = (check_digest[i] & 0x00FF0000) >> 16;
			unsigned char chr3 = (check_digest[i] & 0x0000FF00) >> 8;
			unsigned char chr4 = (check_digest[i] & 0x000000FF);

			code.code_hash = code.code_hash + to_printable_hex(chr1) + to_printable_hex(chr2) +
				to_printable_hex(chr3) + to_printable_hex(chr4);
		}

		int api_count = 0;
		char* api_buf = nullptr;

		INIT_API_FROM_FILE(code.abi);
		INIT_API_FROM_FILE(code.offline_abi);
		INIT_API_FROM_FILE(code.events);

		int storage_count = 0;
		char* storage_buf = nullptr;
		::uvm::blockchain::StorageValueTypes storage_type;

		INIT_STORAGE_FROM_FILE(code.storage_properties);
		
		//read api_arg_types
		int apis_count = 0;
		char api_name_buf[128];
		int args_count = 0;
		UvmTypeInfoEnum arg_type;
		read_count = common_fread_int(f, &apis_count);
		if (read_count == 1) //old gpc version has no api_arg_types
		{
			for (int i = 0; i < apis_count; i++)
			{
				int api_name_len = 0;
				read_count = common_fread_int(f, &api_name_len);
				if (read_count != 1)
				{
					f->common_close();
					throw uvm::core::UvmException("Read verify code fail!");
				}
				read_count = common_fread_octets(f, api_name_buf, api_name_len);
				if (read_count != 1)
				{
					f->common_close();
					throw uvm::core::UvmException("Read verify code fail!");
				}
				if (api_name_len >= 128) {
					f->common_close();
					throw uvm::core::UvmException("api name str len must less than 128");
				}
				api_name_buf[api_name_len] = '\0';
				read_count = common_fread_int(f, (int*)&args_count);
				if (read_count != 1)
				{
					f->common_close();
					throw uvm::core::UvmException("Read verify code fail!");
				}
				std::vector<UvmTypeInfoEnum> types_vector;
				for (int i = 0; i < args_count; i++)
				{
					read_count = common_fread_int(f, (int*)&arg_type);
					if (read_count != 1)
					{
						f->common_close();
						throw uvm::core::UvmException("Read verify code fail!");
					}
					if (arg_type != LTI_STRING && arg_type != LTI_INT && arg_type != LTI_NUMBER && arg_type != LTI_BOOL) { 
						f->common_close();
						throw uvm::core::UvmException("only allow string,int,bool,float arg type!");
					}
					types_vector.push_back(arg_type);
				}
				code.api_arg_types.insert(std::make_pair(std::string(api_name_buf), types_vector));
			}
		}

		// read api args
		{
			int args_apis_count = 0;
			auto read_count = common_fread_int(f, &args_apis_count);
			if (read_count == 1) {
				for (int i = 0; i < args_apis_count; i++)
				{
					int api_len = 0;
					read_count = common_fread_int(f, &api_len);
					if (read_count != 1)
					{
						f->common_close();
						throw uvm::core::UvmException("Read verify code fail!");
					}
					api_buf = (char*)malloc(api_len + 1);
					if (api_buf == NULL)
					{
						f->common_close();
						FC_ASSERT(api_buf == NULL, "malloc fail!");
					}
					read_count = common_fread_octets(f, api_buf, api_len);
					if (read_count != 1)
					{
						f->common_close();
						free(api_buf);
						throw uvm::core::UvmException("Read verify code fail!");
					}
					api_buf[api_len] = '\0';
					std::string api_name(api_buf);
					free(api_buf);
					int args_count = 0;
					read_count = common_fread_int(f, &args_count);
					if (read_count != 1)
					{
						f->common_close();
						free(api_buf);
						throw uvm::core::UvmException("Read verify code fail!");
					}
					std::vector<UvmTypeInfoEnum> api_args;
					for (int j = 0; j < args_count; j++) {
						int arg_type = 0;
						read_count = common_fread_int(f, (int*)&arg_type);
						if (read_count != 1)
						{
							f->common_close();
							throw uvm::core::UvmException("Read verify code fail!");
						}
						api_args.push_back(static_cast<UvmTypeInfoEnum>(arg_type));
					}
					code.api_arg_types[api_name] = api_args;
				}
			}
		}

		f->common_close();
		return code;
	}

	uvm::blockchain::Code ContractHelper::load_contract_from_file(const fc::path &path)
	{
		if (!fc::exists(path))
			FC_THROW_EXCEPTION(fc::file_not_found_exception, "Script file not found!");
		string file = path.string();
		FILE* f = fopen(file.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		int fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		FileWrapper fw(f);

		return load_contract_from_common_stream_and_close(&fw, fsize);
	}
}
