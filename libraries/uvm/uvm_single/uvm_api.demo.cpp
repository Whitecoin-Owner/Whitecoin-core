/**
* lua module injector header in uvm
*/

#include <uvm/lprefix.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <sstream>
#include <utility>
#include <list>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>
#include <uvm/uvm_lutil.h>
#include <uvm/lobject.h>
#include <uvm/lstate.h>
#include <uvm/exceptions.h>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/hex.hpp>
#include "Keccak.hpp"
#include "uvm_api.demo.h"

#include <fc/io/json.hpp>
#include <simplechain/storage.h>



namespace uvm {
	namespace lua {
		namespace api {
			// demo，API

			static int has_error = 0;

			static std::string get_file_name_str_from_contract_module_name(std::string name)
			{
				std::stringstream ss;
				ss << "uvm_contract_" << name;
				return ss.str();
			}

			/**
			* whether exception happen in L
			*/
			bool DemoUvmChainApi::has_exception(lua_State *L)
			{
				return has_error ? true : false;
			}

			/**
			* clear exception marked
			*/
			void DemoUvmChainApi::clear_exceptions(lua_State *L)
			{
				has_error = 0;
			}

			/**
			* when exception happened, use this api to tell uvm
			* @param L the lua stack
			* @param code error code, 0 is OK, other is different error
			* @param error_format error info string, will be released by lua
			* @param ... error arguments
			*/
			void DemoUvmChainApi::throw_exception(lua_State *L, int code, const char *error_format, ...)
			{
				has_error = 1;
				char *msg = (char*)malloc(sizeof(char)*(LUA_VM_EXCEPTION_STRNG_MAX_LENGTH +1));
				if(!msg)
				{
					perror("malloc error");
					return;
				}
				memset(msg, 0x0, LUA_VM_EXCEPTION_STRNG_MAX_LENGTH +1);

				va_list vap;
				va_start(vap, error_format);
				// printf(error_format, vap);
				// const char *msg = luaO_pushfstring(L, error_format, vap);
				vsnprintf(msg, LUA_VM_EXCEPTION_STRNG_MAX_LENGTH, error_format, vap);
				va_end(vap);
				lua_set_compile_error(L, msg);
				printf("%s\n", msg);
				free(msg);
				// luaL_error(L, error_format); // notify lua error
			}

			/**
			* check whether the contract apis limit over, in this lua_State
			* @param L the lua stack
			* @return TRUE(1 or not 0) if over limit(will break the vm), FALSE(0) if not over limit
			*/
			int DemoUvmChainApi::check_contract_api_instructions_over_limit(lua_State *L)
			{
				return 0;
			}

			int DemoUvmChainApi::get_stored_contract_info(lua_State *L, const char *name, std::shared_ptr<UvmContractInfo> contract_info_ret)
			{
				if (uvm::util::starts_with(name, "@"))
				{
					perror("wrong contract name\n");
					exit(1);
				}
				if(contract_info_ret)
				{
					contract_info_ret->contract_apis.push_back("init");
					contract_info_ret->contract_apis.push_back("start");
				}
				return 1;
			}
			int DemoUvmChainApi::get_stored_contract_info_by_address(lua_State *L, const char *contract_id, std::shared_ptr<UvmContractInfo> contract_info_ret)
			{
				if (uvm::util::starts_with(contract_id, "@"))
				{
					perror("wrong contract address\n");
					exit(1);
				}
				if (contract_info_ret)
				{
					contract_info_ret->contract_apis.push_back("init");
					contract_info_ret->contract_apis.push_back("start");
				}
				return 1;
			}

			std::shared_ptr<UvmModuleByteStream> DemoUvmChainApi::get_bytestream_from_code(lua_State *L, const uvm::blockchain::Code& code)
			{
				return nullptr;
			}

			void DemoUvmChainApi::get_contract_address_by_name(lua_State *L, const char *name, char *address, size_t *address_size)
			{
				std::string name_str(name);
				if (name_str == std::string("not_found"))
					return;
				if(uvm::util::starts_with(name_str, std::string(STREAM_CONTRACT_PREFIX)))
				{
					name_str = name_str.substr(strlen(STREAM_CONTRACT_PREFIX));
				}
				std::string addr_str = std::string("id_") + name_str;
				strcpy(address, addr_str.c_str());
				if(address_size)
					*address_size = addr_str.size() + 1;
			}
            
            bool DemoUvmChainApi::check_contract_exist_by_address(lua_State *L, const char *address)
            {
                return true;
            }

			bool DemoUvmChainApi::check_contract_exist(lua_State *L, const char *name)
			{
				std::string filename = std::string("uvm_modules") + uvm::util::file_separator_str() + "uvm_contract_" + name;
				FILE *f = fopen(filename.c_str(), "rb");
				bool exist = false;
				if (nullptr != f)
				{
					exist = true;
					fclose(f);
				}
				return exist;
			}

			/**
			* load contract lua byte stream from uvm api
			*/
			/*std::shared_ptr<UvmModuleByteStream> DemoUvmChainApi::open_contract(lua_State *L, const char *name)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				bool is_bytes = true;
				std::string filename = std::string("uvm_modules") + uvm::util::file_separator_str() + "uvm_contract_" + name;
				FILE *f = fopen(filename.c_str(), "rb");
				if (nullptr == f)
				{
					std::string origin_filename(filename);
					filename = origin_filename + ".lua";
					f = fopen(filename.c_str(), "rb");
					if (!f)
					{
						filename = origin_filename + ".glua";
						f = fopen(filename.c_str(), "rb");
						if(nullptr == f)
							return nullptr;
					}
					is_bytes = false;
				}
				auto stream = std::make_shared<UvmModuleByteStream>();
                if(nullptr == stream)
                    return nullptr;
				fseek(f, 0, SEEK_END);
				auto file_size = ftell(f);
				stream->buff.resize(file_size);
				fseek(f, 0, 0);
				file_size = (long) fread(stream->buff.data(), file_size, 1, f);
				fseek(f, 0, SEEK_END); // seek to end of file
				file_size = ftell(f); // get current file pointer
				stream->is_bytes = is_bytes;
				stream->contract_name = name;
				stream->contract_id = std::string("id_") + std::string(name);
				fclose(f);
				if (!is_bytes)
					stream->buff[stream->buff.size()-1] = '\0';
				return stream;
			}*/

			std::shared_ptr<UvmModuleByteStream> DemoUvmChainApi::open_contract(lua_State *L, const char *name)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				bool is_bytes = true;
				
				//..............................................	
				std::string filename = std::string(name);
				if (filename.find(uvm::util::file_separator_str()[0]) == std::string::npos) { // for test
					filename = std::string("uvm_modules") + uvm::util::file_separator_str() + "uvm_contract_" + name;
				}

				FILE *f = fopen(filename.c_str(), "rb");
				if (nullptr == f)
				{
					std::string origin_filename(name);
					char filesep = uvm::util::file_separator_str()[0];
					int pos = origin_filename.find('/');
					while (pos > 0) {
						origin_filename.replace(pos, 1, "\\");
						pos = origin_filename.find('/');
					}
					filename = origin_filename;
					f = fopen(filename.c_str(), "rb");
					if(nullptr == f){
						filename = origin_filename + ".lua";
						f = fopen(filename.c_str(), "rb");
						if (!f)
						{
							filename = origin_filename + ".glua";
							f = fopen(filename.c_str(), "rb");
							if (nullptr == f)
								return nullptr;
						}
					}
				}
				auto stream = std::make_shared<UvmModuleByteStream>();
				if (nullptr == stream)
					return nullptr;
				fseek(f, 0, SEEK_END);
				auto file_size = ftell(f);
				stream->buff.resize(file_size);
				fseek(f, 0, 0);
				file_size = (long)fread(stream->buff.data(), file_size, 1, f);
				fseek(f, 0, SEEK_END); // seek to end of file
				file_size = ftell(f); // get current file pointer
				stream->is_bytes = is_bytes;
				stream->contract_name = name;
				stream->contract_id = std::string("id_") + std::string(name);
				fclose(f);
				if (!is_bytes)
					stream->buff[stream->buff.size() - 1] = '\0';
				//........................................


				//open meta json
				filename =  std::string(name) + ".meta.json";
				f = fopen(filename.c_str(), "rb");
				if (nullptr == f) {
					filename = std::string(name);
					auto pos = filename.find_last_of('.');
					
					if (pos >= 0) {
						filename = filename.substr(0, pos) + ".meta.json";
						f = fopen(filename.c_str(), "rb");
						if (nullptr != f) {
							fclose(f);
							auto meta = fc::json::from_file(fc::path(filename), fc::json::legacy_parser);
							if (!meta.is_object()) {
								return stream;
							}
							auto metajson = meta.as<fc::mutable_variant_object>();
							
							if (metajson["storage_properties_types"].is_array()) {
								auto storage_types_json_array = metajson["storage_properties_types"].as<fc::variants>();
								for (size_t i = 0; i < storage_types_json_array.size(); i++)
								{		
									auto item_json = storage_types_json_array[i].as<fc::variants>();
									if (item_json.size() == 2) {
										stream->contract_storage_properties[item_json[0].as_string()] = uvm::blockchain::StorageValueTypes(item_json[1].as_uint64());
									}
									
								}
							}
						}
					}
				}
				return stream;				
			}
            
			std::shared_ptr<UvmModuleByteStream> DemoUvmChainApi::open_contract_by_address(lua_State *L, const char *address)
            {
				if (uvm::util::starts_with(address, "id_"))
				{
					std::string address_str = address;
					auto name = address_str.substr(strlen("id_"));
					if(name.length()>=6 && name[0]>='0' && name[0]<='9')
					{
						// stream
						auto addr_pointer = std::atoll(name.c_str());
						auto stream = (UvmModuleByteStream*)addr_pointer;
						auto result_stream = std::make_shared<UvmModuleByteStream>();
						*result_stream = *stream;
						return result_stream;
					}
					return open_contract(L, name.c_str());
				}
                //return open_contract(L, "pointer_demo");
				return open_contract(L, address);
            }

            // storage,mapkey contract_id + "$" + storage_name
            // TODO: lua_closepost_callback，
            static std::map<lua_State *, std::shared_ptr<std::map<std::string, UvmStorageValue>>> _demo_chain_storage_buffer;
			static fc::mutable_variant_object storage_root;
			static bool is_init_storage_file = false;

			UvmStorageValue DemoUvmChainApi::get_storage_value_from_uvm(lua_State *L, const char *contract_name,
				const std::string& name, const std::string& fast_map_key, bool is_fast_map)
			{
				if (!is_init_storage_file) {
					if (fc::exists(fc::path("uvm_storage_demo.json"))) {
						storage_root = fc::json::from_file(fc::path("uvm_storage_demo.json"), fc::json::legacy_parser).as<fc::mutable_variant_object>();
					}					
					is_init_storage_file = true;
				}

				std::string contract_id = contract_name;
				int pos = contract_id.find('\\');
				while (pos > 0) {
					contract_id.replace(pos, 1, "/");
					pos = contract_id.find('\\');
				}

				auto key = std::string(contract_name) + "$" + name; 
				if (is_fast_map) {
					key = key + "." + fast_map_key;
				}
				if (storage_root.find(key) != storage_root.end()) {
					auto s = simplechain::json_to_uvm_storage_value(L, storage_root[key]);
					return s;
				}
				UvmStorageValue value;
				value.type = uvm::blockchain::StorageValueTypes::storage_value_null;
				value.value.int_value = 0;
				return value;
			}

			UvmStorageValue DemoUvmChainApi::get_storage_value_from_uvm_by_address(lua_State *L, const char *contract_address,
				const std::string& name, const std::string& fast_map_key, bool is_fast_map)
			{
				if (!is_init_storage_file) {
					if (fc::exists(fc::path("uvm_storage_demo.json"))) {
						storage_root = fc::json::from_file(fc::path("uvm_storage_demo.json"), fc::json::legacy_parser).as<fc::mutable_variant_object>();
					}
					is_init_storage_file = true;
				}
				std::string contract_id = contract_address;
				int pos = contract_id.find('\\');
				while (pos > 0) {
					contract_id.replace(pos, 1, "/");
					pos = contract_id.find('\\');
				}
				auto key = std::string(contract_id) + "$" + name;
				if (is_fast_map) {
					key = key + "." + fast_map_key;
				}
				if (storage_root.find(key) != storage_root.end()) {
					auto s = simplechain::json_to_uvm_storage_value(L, storage_root[key]);
					return s;
				}
				UvmStorageValue value;
				value.type = uvm::blockchain::StorageValueTypes::storage_value_null;
				value.value.int_value = 0;

				return value;
			}

			bool DemoUvmChainApi::commit_storage_changes_to_uvm(lua_State *L, AllContractsChangesMap &changes)
			{
				if (changes.size() == 0) {
					return true;
				}

				if (!is_init_storage_file) {
					if (fc::exists(fc::path("uvm_storage_demo.json"))) {
						storage_root = fc::json::from_file(fc::path("uvm_storage_demo.json"), fc::json::legacy_parser).as<fc::mutable_variant_object>();
					}
					
					is_init_storage_file = true;
				}

				for (const auto &change : changes)
				{
					auto contract_id = change.first;


					int pos = contract_id.find('\\');
					while (pos > 0) {
						contract_id.replace(pos, 1, "/");
						pos = contract_id.find('\\');
					}
					
					for (const auto &change_info : *(change.second))
					{
						auto name = change_info.first;
						auto change_item = change_info.second;
						
						UvmStorageValue value = change_item.after;
						auto key = contract_id + "$" + name;
						storage_root[key] = simplechain::uvm_storage_value_to_json(value);
					}
				}

				if (changes.size()&& storage_root.size()) {
					/*if (!fc::exists(fc::path("uvm_storage_demo.json"))) {
						auto f = fopen("uvm_storage_demo.json", "w");
						if (f) {
							fclose(f);
						}
					}*/
					//持久化
					fc::variant temp;
					fc::to_variant(storage_root, temp);
					fc::json::save_to_file(temp, fc::path("uvm_storage_demo.json"),true, fc::json::legacy_generator);
					printf("save uvm_storage_demo.json OK!\n");
				}
				return true;
			}

			intptr_t DemoUvmChainApi::register_object_in_pool(lua_State *L, intptr_t object_addr, UvmOutsideObjectTypes type)
			{
				auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
				// Map<type, Map<object_key, object_addr>>
				std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
				if(node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
				{
					node.type = UvmStateValueType::LUA_STATE_VALUE_POINTER;
					object_pools = new std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>>();
					node.value.pointer_value = (void*)object_pools;
					uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, node.value, node.type);
				} 
				else
				{
					object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
				}
				if(object_pools->find(type) == object_pools->end())
				{
					object_pools->emplace(std::make_pair(type, std::make_shared<std::map<intptr_t, intptr_t>>()));
				}
				auto pool = (*object_pools)[type];
				auto object_key = object_addr;
				(*pool)[object_key] = object_addr;
				return object_key;
			}

			intptr_t DemoUvmChainApi::is_object_in_pool(lua_State *L, intptr_t object_key, UvmOutsideObjectTypes type)
			{
				auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
				// Map<type, Map<object_key, object_addr>>
				std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
				if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
				{
					return 0;
				}
				else
				{
					object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
				}
				if (object_pools->find(type) == object_pools->end())
				{
					object_pools->emplace(std::make_pair(type, std::make_shared<std::map<intptr_t, intptr_t>>()));
				}
				auto pool = (*object_pools)[type];
				return (*pool)[object_key];
			}

			void DemoUvmChainApi::release_objects_in_pool(lua_State *L)
			{
				auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
				// Map<type, Map<object_key, object_addr>>
				std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
				if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
				{
					return;
				}
				object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
				// TODO: object_pools，
				for(const auto &p : *object_pools)
				{
					auto type = p.first;
					auto pool = p.second;
					for(const auto &object_item : *pool)
					{
						auto object_key = object_item.first;
						auto object_addr = object_item.second;
						if (object_addr == 0)
							continue;
						switch(type)
						{
						case UvmOutsideObjectTypes::OUTSIDE_STREAM_STORAGE_TYPE:
						{
							auto stream = (uvm::lua::lib::UvmByteStream*) object_addr;
							delete stream;
						} break;
						default: {
							continue;
						}
						}
					}
				}
				delete object_pools;
				UvmStateValue null_state_value;
				null_state_value.int_value = 0;
				uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, null_state_value, UvmStateValueType::LUA_STATE_VALUE_nullptr);
			}

			bool DemoUvmChainApi::register_storage(lua_State *L, const char *contract_name, const char *name)
			{
				// printf("registered storage %s[%s] to uvm\n", contract_name, name);
				return true;
			}

			lua_Integer DemoUvmChainApi::transfer_from_contract_to_address(lua_State *L, const char *contract_address, const char *to_address,
				const char *asset_type, int64_t amount_str)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				printf("contract transfer from %s to %s, asset[%s] amount %ld\n", contract_address, to_address, asset_type, amount_str);
				return 0;
			}

			lua_Integer DemoUvmChainApi::transfer_from_contract_to_public_account(lua_State *L, const char *contract_address, const char *to_account_name,
				const char *asset_type, int64_t amount)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				printf("contract transfer from %s to %s, asset[%s] amount %ld\n", contract_address, to_account_name, asset_type, amount);
				return 0;
			}

			int64_t DemoUvmChainApi::get_contract_balance_amount(lua_State *L, const char *contract_address, const char* asset_symbol)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				return 0;
			}

			int64_t DemoUvmChainApi::get_transaction_fee(lua_State *L)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				return 0;
			}

			uint32_t DemoUvmChainApi::get_chain_now(lua_State *L)
			{				
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				return 0;
			}

			uint32_t DemoUvmChainApi::get_chain_random(lua_State *L)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				return 0;
			}

			uint32_t DemoUvmChainApi::get_chain_safe_random(lua_State *L, bool diff_in_diff_txs) {
				return get_chain_random(L);
			}

			std::string DemoUvmChainApi::get_transaction_id(lua_State *L)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			  return get_transaction_id_without_gas(L);
			}

			std::string DemoUvmChainApi::get_transaction_id_without_gas(lua_State *L) const {
				return "";
			}

			uint32_t DemoUvmChainApi::get_header_block_num(lua_State *L)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				return 0;
			}

			uint32_t DemoUvmChainApi::get_header_block_num_without_gas(lua_State *L) const
			{
				return 0;
			}

			uint32_t DemoUvmChainApi::wait_for_future_random(lua_State *L, int next)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				return 0;
			}

			int32_t DemoUvmChainApi::get_waited(lua_State *L, uint32_t num)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				return num;
			}

			void DemoUvmChainApi::emit(lua_State *L, const char* contract_id, const char* event_name, const char* event_param)
			{
              uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				printf("emit called\n");
			}

			bool DemoUvmChainApi::is_valid_address(lua_State *L, const char *address_str)
			{
				return true;
			}

			bool DemoUvmChainApi::is_valid_contract_address(lua_State *L, const char *address_str)
			{
				if (std::string(address_str).find_first_of("CON_") == 0) {
					return true;
				}
				return false;
			}

			const char * DemoUvmChainApi::get_system_asset_symbol(lua_State *L)
			{
				return "COIN";
			}

			uint64_t DemoUvmChainApi::get_system_asset_precision(lua_State *L)
			{
				return 10000;
			}

			static std::vector<char> hex_to_chars(const std::string& hex_string) {
				std::vector<char> chars(hex_string.size() / 2);
				auto bytes_count = fc::from_hex(hex_string, chars.data(), chars.size());
				if (bytes_count != chars.size()) {
					throw uvm::core::UvmException("parse hex to bytes error");
				}
				return chars;
			}

			std::vector<unsigned char> DemoUvmChainApi::hex_to_bytes(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				std::vector<unsigned char> bytes(chars.size());
				memcpy(bytes.data(), chars.data(), chars.size());
				return bytes;
			}
			std::string DemoUvmChainApi::bytes_to_hex(std::vector<unsigned char> bytes) {
				std::vector<char> chars(bytes.size());
				memcpy(chars.data(), bytes.data(), bytes.size());
				return fc::to_hex(chars);
			}
			std::string DemoUvmChainApi::sha256_hex(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = fc::sha256::hash(chars.data(), chars.size());
				return hash_result.str();
			}
			std::string DemoUvmChainApi::sha1_hex(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = fc::sha1::hash(chars.data(), chars.size());
				return hash_result.str();
			}
			std::string DemoUvmChainApi::sha3_hex(const std::string& hex_string) {
				Keccak keccak(Keccak::Keccak256);
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = keccak(chars.data(), chars.size());
				return hash_result;
			}
			std::string DemoUvmChainApi::ripemd160_hex(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = fc::ripemd160::hash(chars.data(), chars.size());
				return hash_result.str();
			}

			std::string DemoUvmChainApi::get_address_role(lua_State* L, const std::string& addr) {
				return "address";
			}

			int64_t DemoUvmChainApi::get_fork_height(lua_State* L, const std::string& fork_key) {
				return 1;
			}

			bool DemoUvmChainApi::use_cbor_diff(lua_State* L) const {
				if (1 == L->cbor_diff_state) {
					return true;
				}
				else if (2 == L->cbor_diff_state) {
					return false;
				}
				auto result = true;
				L->cbor_diff_state = result ? 1 : 2; // cache it
				return result;
			}

			std::string DemoUvmChainApi::pubkey_to_address_string(const fc::ecc::public_key& pub) const {
				auto dat = pub.serialize();
				auto addr = fc::ripemd160::hash(fc::sha512::hash(dat.data, sizeof(dat)));
				std::string prefix = "SL";
				unsigned char version = 0X35;
				fc::array<char, 25> bin_addr;
				memcpy((char*)&bin_addr, (char*)&version, sizeof(version));
				memcpy((char*)&bin_addr + 1, (char*)&addr, sizeof(addr));
				auto checksum = fc::ripemd160::hash((char*)&bin_addr, bin_addr.size() - 4);
				memcpy(((char*)&bin_addr) + 21, (char*)&checksum._hash[0], 4);
				return prefix + fc::to_base58(bin_addr.data, sizeof(bin_addr));
			}

		}
	}
}
