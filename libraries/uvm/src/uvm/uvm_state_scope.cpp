#include <uvm/lprefix.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <utility>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>

#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>

#include <uvm/uvm_lutil.h>
#include <uvm/lobject.h>
#include <uvm/lzio.h>
#include <uvm/lundump.h>
#include <uvm/lvm.h>
#include <uvm/lapi.h>
#include <uvm/lopcodes.h>
#include <uvm/lparser.h>
#include <uvm/lstate.h>
#include <uvm/ldo.h>
#include <uvm/ldebug.h>
#include <uvm/lauxlib.h>
#include <uvm/lualib.h>

using uvm::lua::api::global_uvm_chain_api;

namespace uvm
{
    namespace lua
    {
        namespace lib
        {
			GasManager::GasManager(lua_State* L): _L(L) {}

			void GasManager::add_gas(int64_t gas) {
				auto ref = gas_ref_or_new();
				if (nullptr == ref) {
					return;
				}
				*ref += gas;
			}

			int64_t GasManager::gas() const {
				auto ref = gas_ref();
				if (nullptr == ref) {
					return 0;
				}
				return *ref;
			}

			int64_t* GasManager::gas_ref() const {
				int64_t *insts_executed_count = uvm::lua::lib::get_lua_state_value(_L, INSTRUCTIONS_EXECUTED_COUNT_LUA_STATE_MAP_KEY).int_pointer_value;
				return insts_executed_count;
			}

			int64_t* GasManager::gas_ref_or_new() {
				int64_t* ref = gas_ref();
				if (nullptr == ref)
				{
					ref = static_cast<int64_t*>(lua_malloc(_L, sizeof(int64_t)));
					if (nullptr == ref) {
						return nullptr;
					}
					*ref = 0;
					UvmStateValue lua_state_value_of_exected_count;
					lua_state_value_of_exected_count.int_pointer_value = ref;
					uvm::lua::lib::set_lua_state_value(_L, INSTRUCTIONS_EXECUTED_COUNT_LUA_STATE_MAP_KEY, lua_state_value_of_exected_count, LUA_STATE_VALUE_INT_POINTER);
				}
				return ref;
			}

            UvmStateScope::UvmStateScope(bool use_contract)
                :_use_contract(use_contract) {
                this->_L = create_lua_state(use_contract);
            }
            UvmStateScope::UvmStateScope(const UvmStateScope &other) : _L(other._L) {}
            UvmStateScope::~UvmStateScope() {
				if (nullptr != _L)
                    close_lua_state(_L);
            }

            void UvmStateScope::change_in_file(FILE *in)
            {
                if (nullptr == in)
                    in = stdin;
                _L->in = in;
            }

            void UvmStateScope::change_out_file(FILE *out)
            {
                if (nullptr == out)
                    out = stdout;
                _L->out = out;
            }

            void UvmStateScope::change_err_file(FILE *err)
            {
                if (nullptr == err)
                    err = stderr;
                _L->err = err;
            }

            bool UvmStateScope::in_sandbox() const {
                return check_in_lua_sandbox(_L);
            }

            void UvmStateScope::enter_sandbox() {
                enter_lua_sandbox(_L);
            }

            void UvmStateScope::exit_sandbox() {
                exit_lua_sandbox(_L);
            }

            bool UvmStateScope::commit_storage_changes()
            {
                return uvm::lua::lib::commit_storage_changes(_L);
            }

            UvmStateValueNode UvmStateScope::get_value_node(const char *key)
            {
                return get_lua_state_value_node(_L, key);
            }
            UvmStateValue UvmStateScope::get_value(const char *key)
            {
                return get_lua_state_value(_L, key);
            }

            void UvmStateScope::set_value(const char *key, UvmStateValue value, enum UvmStateValueType type)
            {
                set_lua_state_value(_L, key, value, type);
            }

            void UvmStateScope::set_instructions_limit(int64_t limit)
            {
                return set_lua_state_instructions_limit(_L, limit);
            }
			int64_t UvmStateScope::get_instructions_limit() const
            {
                return get_lua_state_instructions_limit(_L);
            }
            int UvmStateScope::get_instructions_executed_count() const
            {
                return get_lua_state_instructions_executed_count(_L);
            }
            int UvmStateScope::check_uvm_contract_api_instructions_over_limit()
            {
                return global_uvm_chain_api->check_contract_api_instructions_over_limit(_L);
            }

            void UvmStateScope::notify_stop()
            {
                notify_lua_state_stop(_L);
            }
            bool UvmStateScope::check_notified_stop()
            {
                return check_lua_state_notified_stop(_L);
            }
            void UvmStateScope::resume_running()
            {
                resume_lua_state_running(_L);
            }

            bool UvmStateScope::run_compiledfile(const char *filename)
            {
                return uvm::lua::lib::run_compiledfile(_L, filename);
            }
            bool UvmStateScope::run_compiled_bytestream(void *stream_addr)
            {
                return uvm::lua::lib::run_compiled_bytestream(_L, stream_addr);
            }
            void UvmStateScope::add_global_c_function(const char *name, lua_CFunction func)
            {
                uvm::lua::lib::add_global_c_function(_L, name, func);
            }
            void UvmStateScope::add_global_string_variable(const char *name, const char *str)
            {
                uvm::lua::lib::add_global_string_variable(_L, name, str);
            }
            void UvmStateScope::add_global_int_variable(const char *name, lua_Integer num)
            {
                uvm::lua::lib::add_global_int_variable(_L, name, num);
            }
            void UvmStateScope::add_global_number_variable(const char *name, lua_Number num)
            {
                uvm::lua::lib::add_global_number_variable(_L, name, num);
            }
            void UvmStateScope::add_global_bool_variable(const char *name, bool value)
            {
                uvm::lua::lib::add_global_bool_variable(_L, name, value);
            }
            void UvmStateScope::register_module(const char *name, lua_CFunction openmodule_func)
            {
                uvm::lua::lib::register_module(_L, name, openmodule_func);
            }
            int UvmStateScope::execute_contract_api(const char *contract_name, const char *api_name, cbor::CborArrayValue& args, std::string *result_json_string)
            {
                return uvm::lua::lib::execute_contract_api(_L, contract_name, api_name, args, result_json_string);
            }

            int UvmStateScope::execute_contract_api_by_address(const char *address, const char *api_name, cbor::CborArrayValue& args, std::string *result_json_string)
            {
                return uvm::lua::lib::execute_contract_api_by_address(_L, address, api_name, args, result_json_string);
            }

            bool UvmStateScope::execute_contract_init_by_address(const char *contract_address, cbor::CborArrayValue& args, std::string *result_json_string)
            {
                return uvm::lua::lib::execute_contract_init_by_address(_L, contract_address, args, result_json_string);
            }
            bool UvmStateScope::execute_contract_start_by_address(const char *contract_address, cbor::CborArrayValue& args, std::string *result_json_string)
            {
                return uvm::lua::lib::execute_contract_start_by_address(_L, contract_address, args, result_json_string);
            }

            bool UvmStateScope::execute_contract_init(UvmModuleByteStream *stream, cbor::CborArrayValue& args, std::string *result_json_string)
            {
                return uvm::lua::lib::execute_contract_init(_L, "tmp", stream, args, result_json_string);
            }
            bool UvmStateScope::execute_contract_start(UvmModuleByteStream *stream, cbor::CborArrayValue& args, std::string *result_json_string)
            {
                return uvm::lua::lib::execute_contract_start(_L, "tmp", stream, args, result_json_string);
            }

            bool UvmStateScope::check_contract_bytecode_stream(UvmModuleByteStream *stream)
            {
                return uvm::lua::lib::check_contract_bytecode_stream(_L, stream);
            }

			void UvmStateScope::add_system_extra_libs()
            {
				uvm::lua::lib::add_system_extra_libs(_L);
            }

        }
    }
}
