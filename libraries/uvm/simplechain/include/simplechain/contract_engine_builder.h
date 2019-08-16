#pragma once
#include <simplechain/config.h>
#include <simplechain/contract_entry.h>
#include <simplechain/uvm_contract_engine.h>
#include <uvm/uvm_lib.h>

namespace simplechain {
	typedef UvmContractEngine ActiveContractEngine;

	class ContractEngineBuilder
	{
	private:
		std::shared_ptr<ActiveContractEngine> _engine;
	public:
		virtual ~ContractEngineBuilder();
		ContractEngineBuilder *set_use_contract(bool use_contract);
		ContractEngineBuilder *set_caller(std::string caller, std::string caller_address);
		std::shared_ptr<ActiveContractEngine> build();
	};
}
