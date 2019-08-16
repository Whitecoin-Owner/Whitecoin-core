#include <simplechain/contract_entry.h>
#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/hex.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>

#define PRINTABLE_CHAR(chr) \
if (chr >= 0 && chr <= 9)  \
    chr = chr + '0'; \
else \
    chr = chr + 'a' - 10; 

static std::string to_printable_hex(unsigned char chr)
{
	unsigned char high = chr >> 4;
	unsigned char low = chr & 0x0F;
	char tmp[16];

	PRINTABLE_CHAR(high);
	PRINTABLE_CHAR(low);

	snprintf(tmp, sizeof(tmp), "%c%c", high, low);
	return std::string(tmp);
}

namespace uvm {
	namespace blockchain {
		using namespace simplechain;

		std::string Code::GetHash() const
		{
			std::string hashstr = "";
			boost::uuids::detail::sha1 sha;
			unsigned int check_digest[5];
			sha.process_bytes(code.data(), code.size());
			sha.get_digest(check_digest);
			for (int i = 0; i < 5; ++i)
			{
				unsigned char chr1 = (check_digest[i] & 0xFF000000) >> 24;
				unsigned char chr2 = (check_digest[i] & 0x00FF0000) >> 16;
				unsigned char chr3 = (check_digest[i] & 0x0000FF00) >> 8;
				unsigned char chr4 = (check_digest[i] & 0x000000FF);

				hashstr = hashstr + to_printable_hex(chr1) + to_printable_hex(chr2) +
					to_printable_hex(chr3) + to_printable_hex(chr4);
			}
			return hashstr;
		}

		bool Code::operator!=(const Code& it) const
		{
			if (abi != it.abi)
				return true;
			if (offline_abi != it.offline_abi)
				return true;
			if (events != it.events)
				return true;
			if (storage_properties != it.storage_properties)
				return true;
			if (code != it.code)
				return true;
			if (code_hash != code_hash)
				return true;
			return false;
		}

		void Code::SetApis(char* module_apis[], int count, int api_type)
		{
			if (api_type == ContractApiType::chain)
			{
				abi.clear();
				for (int i = 0; i < count; i++)
					abi.insert(module_apis[i]);
			}
			else if (api_type == ContractApiType::offline)
			{
				offline_abi.clear();
				for (int i = 0; i < count; i++)
					offline_abi.insert(module_apis[i]);
			}
			else if (api_type == ContractApiType::event)
			{
				events.clear();
				for (int i = 0; i < count; i++)
					events.insert(module_apis[i]);
			}
		}

		bool Code::valid() const
		{
			//FC_ASSERT(0);
			//return false;
			return true;
		}
	}
}

namespace simplechain {
	CodePrintAble::CodePrintAble(const uvm::blockchain::Code& code) : abi(code.abi), offline_abi(code.offline_abi), events(code.events), code_hash(code.code_hash)
	{
		unsigned char* code_buf = (unsigned char*)malloc(code.code.size());
		FC_ASSERT(code_buf, "malloc failed");

		for (int i = 0; i < code.code.size(); ++i)
		{
			code_buf[i] = code.code[i];
			printable_code = printable_code + to_printable_hex(code_buf[i]);
		}

		free(code_buf);
	}
}

namespace fc {
	void from_variant(const fc::variant& var, simplechain::contract_type& vo) {
		auto varstr = var.as_string();
		if (varstr == "normal_contract") {
			vo = simplechain::contract_type::normal_contract;
		}
		else if (varstr == "native_contract") {
			vo = simplechain::contract_type::native_contract;
		}
		else if (varstr == "contract_based_on_template") {
			vo = simplechain::contract_type::contract_based_on_template;
		}
	}

	void from_variant(const fc::variant& var, std::vector<unsigned char>& vo) {
		const auto& varstr = var.as_string();
		vo.resize(varstr.size() / 2);
		fc::from_hex(varstr, (char*) vo.data(), vo.size());
	}

	void from_variant(const fc::variant& var, fc::enum_type<fc::unsigned_int, uvm::blockchain::StorageValueTypes>& vo) {
		vo = var.as_int64();
	}

	void from_variant(const fc::variant& var, uvm::blockchain::Code& vo) {
		const auto& varobj = var.as<fc::mutable_variant_object>();
		if (varobj.find("code") != varobj.end())
			fc::from_variant(varobj["code"], vo.code);
		else if (varobj.find("printable_code") != varobj.end())
			fc::from_variant(varobj["printable_code"], vo.code);
		
		fc::from_variant(varobj["abi"], vo.abi);
		fc::from_variant(varobj["offline_abi"], vo.offline_abi);
		fc::from_variant(varobj["events"], vo.events);
		fc::from_variant(varobj["printable_storage_properties"], vo.storage_properties);
		// TODO: fc::from_variant(varobj["contract_api_arg_types"], vo.contract_api_arg_types);
		fc::from_variant(varobj["code_hash"], vo.code_hash);
	}
}
