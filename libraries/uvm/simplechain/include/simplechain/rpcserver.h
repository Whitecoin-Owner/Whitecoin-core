#pragma once
#include <simplechain/config.h>
#include <simplechain/blockchain.h>
#include <simplechain/transaction.h>
#include <simplechain/contract.h>
#include <simplechain/operation.h>
#include <simplechain/contract_helper.h>
#include <simplechain/operations_helper.h>
#include <simplechain/chain_rpc.h>
#include <server_http.hpp>

namespace simplechain {

	class RpcServer final {
	private:
		blockchain* _chain;
		int _port;
		std::shared_ptr<HttpServer> _server;
	public:
		RpcServer(blockchain* chain, int port = 8080);
		~RpcServer();

		void start();
	};
}
