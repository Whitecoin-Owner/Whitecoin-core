#include <simplechain/contract_object.h>
#include <simplechain/contract_entry.h>

namespace simplechain {
	ContractEntryPrintable contract_object::to_printable(const contract_object& obj) {
		ContractEntryPrintable printable;
		printable.id = obj.contract_address;
		printable.owner_address = obj.owner_address;
		printable.name = obj.contract_name;
		printable.description = obj.contract_desc;
		printable.type_of_contract = obj.type_of_contract;
		printable.code_printable = obj.code;
		printable.registered_block = obj.registered_block;
		if (obj.inherit_from != "")
		{
			printable.inherit_from = obj.inherit_from;
		}
		for (auto& addr : obj.derived)
		{
			printable.derived.push_back(addr);
		}
		printable.createtime = obj.create_time;
		return printable;
	}
}

namespace fc {
	void to_variant(const simplechain::contract_object& var, variant& vo) {
		fc::mutable_variant_object obj("registered_block", var.registered_block);
		obj("owner_address", var.owner_address);
		// TODO: code to varient
		obj("create_time", var.create_time);
		obj("contract_address", var.contract_address);
		obj("contract_name", var.contract_name);
		obj("contract_desc", var.contract_desc);
		obj("type_of_contract", var.type_of_contract);
		obj("native_contract_key", var.native_contract_key);
		obj("derived", var.derived);
		obj("inherit_from", var.inherit_from);
		vo = std::move(obj);
	}
	void from_variant(const fc::variant& var, simplechain::contract_object& vo) {
		const auto& varobj = var.as<fc::mutable_variant_object>();
		vo.registered_block = varobj["registered_block"].as_uint64();
		vo.owner_address = varobj["owner_address"].as_string();
		
		if (varobj.find("code") != varobj.end())
			fc::from_variant(varobj["code"], vo.code);
		else if (varobj.find("code_printable") != varobj.end())
			fc::from_variant(varobj["code_printable"], vo.code);
		// fc::from_variant(varobj["create_time"], vo.create_time);
		if (varobj.find("contract_address") != varobj.end())
			vo.contract_address = varobj["contract_address"].as_string();
		else if(varobj.find("id") != varobj.end())
			vo.contract_address = varobj["id"].as_string();
		if(varobj.find("contract_name") != varobj.end())
			vo.contract_name = varobj["contract_name"].as_string();
		if(varobj.find("contract_desc") != varobj.end())
			vo.contract_desc = varobj["contract_desc"].as_string();
		if(varobj.find("type_of_contract") != varobj.end())
			fc::from_variant(varobj["type_of_contract"], vo.type_of_contract);
		if(varobj.find("native_contract_key") != varobj.end())
			vo.native_contract_key = varobj["native_contract_key"].as_string();
		if(varobj.find("derived") != varobj.end())
			vo.derived = varobj["derived"].as <std::vector<std::string>>();
		if(varobj.find("inherit_from") != varobj.end())
			vo.inherit_from = varobj["inherit_from"].as_string();
	}
}
