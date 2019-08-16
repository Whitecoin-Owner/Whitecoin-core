#pragma once
#include <simplechain/config.h>
#include <simplechain/transaction.h>
#include <simplechain/block.h>
#include <simplechain/evaluator.h>
#include <simplechain/evaluate_result.h>
#include <simplechain/evaluate_state.h>
#include <simplechain/contract_evaluate.h>
#include <simplechain/contract_object.h>
#include <simplechain/storage.h>
#include <simplechain/asset.h>
#include <simplechain/debugger.h>
#include <memory>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <uvm/lobject.h>

namespace simplechain {
	typedef int64_t balance_t; //int64

	class blockchain {
		// TODO: local db and rollback
	private:
		std::vector<asset> assets;
		std::vector<block> blocks;
		std::map<std::string, transaction_receipt> tx_receipts; // txid => tx_receipt
		std::map<std::string, std::string> address_pubkeys; // address => pub_key_hex
		std::map<std::string, std::map<asset_id_t, balance_t> > account_balances;
		std::map<std::string, contract_object> contracts;
		std::map<std::string, std::map<std::string, StorageDataType> > contract_storages;
		std::vector<transaction> tx_mempool;

		std::map<std::string, std::list<uint32_t> > breakpoints;

		std::shared_ptr<generic_evaluator> last_evaluator_when_debugger;
	public:
		blockchain();
		// @throws exception
		std::shared_ptr<evaluate_result> evaluate_transaction(std::shared_ptr<transaction> tx);
		void clear_debugger_info();
		void apply_transaction(std::shared_ptr<transaction> tx);
		block latest_block() const;
		uint64_t head_block_number() const;
		std::string head_block_hash() const;
		std::shared_ptr<transaction> get_trx_by_hash(const std::string& tx_hash) const;
		std::shared_ptr<block> get_block_by_number(uint64_t num) const;
		std::shared_ptr<block> get_block_by_hash(const std::string& block_hash) const;
		balance_t get_account_asset_balance(const std::string& account_address, asset_id_t asset_id) const;
		std::map<asset_id_t, balance_t> get_account_balances(const std::string& account_address) const;
		void update_account_asset_balance(const std::string& account_address, asset_id_t asset_id, int64_t balance_change);
		std::shared_ptr<contract_object> get_contract_by_address(const std::string& addr) const;
		std::shared_ptr<contract_object> get_contract_by_name(const std::string& name) const;
		void store_contract(const std::string& addr, const contract_object& contract_obj);
		StorageDataType get_storage(const std::string& contract_address, const std::string& key) const;
		std::map<std::string, StorageDataType> get_contract_storages(const std::string& contract_address) const;
		void set_storage(const std::string& contract_address, const std::string& key, const StorageDataType& value);
		void add_asset(const asset& new_asset);
		std::shared_ptr<asset> get_asset(asset_id_t asset_id);
		std::shared_ptr<asset> get_asset_by_symbol(const std::string& symbol);
		std::vector<asset> get_assets() const;
		std::vector<contract_object> get_contracts() const;
		std::vector<std::string> get_account_addresses() const;
		void set_tx_receipt(const std::string& tx_id, const transaction_receipt& tx_receipt);
		std::shared_ptr<transaction_receipt> get_tx_receipt(const std::string& tx_id);

		void register_account(const std::string& addr, const std::string& pub_key_hex);
		std::string get_address_pubkey_hex(const std::string& addr) const;
		void load_contract_state(const std::string& contract_addr, const std::string& contract_state_json_str);
		/**
		 * 从json格式的合约信息中创建新合约，但是不调用init，只恢复数据，之后需要再load_contract_state恢复数据状态。用来复原正式链上合约状态以及测试调试
		 * {"id":"",
		    "owner_address":"",
			"owner_name":"",
			"name":"",
			"description":"",
			"type_of_contract":"normal_contract",
			"registered_block": 0,
			"registered_trx":"",
			"native_contract_key":"",
			"derived":[],
			"code_printable": {
			   "abi":["init", "xxxx"],
			   "offline_abi":["xxxx"],
			   "events":["xxxx"],
			   "printable_storage_properties":[],
			   "printable_code":"",
			   "code_hash":""
			 },
		    "createtime":"2019-03-18T10:24:01"}
		 * @throws std::exception
		 */
		std::string load_new_contract_from_json(const std::string& contract_info_json_str);

		void accept_transaction_to_mempool(const transaction& tx);
		std::vector<transaction> get_tx_mempool() const;
		void generate_block();

		fc::variant get_state() const;
		std::string get_state_json() const;

		bool is_break_when_last_evaluate() const;
		void debugger_go_resume();
		void debugger_step_into();
		void debugger_step_out();
		void debugger_step_over();

		void add_breakpoint_in_last_debugger_state(const std::string& contract_address, uint32_t line);
		void remove_breakpoint_in_last_debugger_state(const std::string& contract_address, uint32_t line);
		void clear_breakpoints_in_last_debugger_state();
		std::map<std::string, std::list<uint32_t> > get_breakpoints_in_last_debugger_state();

		std::map<std::string, TValue> view_localvars_in_last_debugger_state() const;
		std::map<std::string, TValue> view_upvalues_in_last_debugger_state() const;
		debugger_state view_debugger_state() const;

		std::pair<std::string, std::string> view_current_contract_stack_item_in_last_debugger_state() const; // return contract_address => api_name
		uint32_t view_current_line_number_in_last_debugger_state() const;
		TValue view_contract_storage_value(const char *name, const char* fast_map_key, bool is_fast_map) const;
		std::vector<std::string> view_call_stack() const;


	private:
		// @throws exception
		std::shared_ptr<generic_evaluator> get_operation_evaluator(transaction* tx, const operation& op);
	};
}
