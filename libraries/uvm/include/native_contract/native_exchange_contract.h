#pragma once

#include <native_contract/native_contract_api.h>

namespace uvm {
	namespace contract {
		//order: {orderInfo:{purchaseAsset:"BTC",purchaseNum:20,payAsset:"XWC",payNum:100,nounce:"13df",relayer:"XWCsrsfsfe3",fee:"0.0001"},sig:"rsowor233"}
		//{takerOrder:order,takerPayNum:80,fillOrders:[{order:order1,fillNum:30},{order:order2,fillNum:50}]
		namespace exchange {
			struct OrderInfo {
				std::string purchaseAsset;
				int64_t purchaseNum;
				std::string payAsset;
				int64_t payNum;
				std::string nonce;
				std::string relayer;
				std::string fee;
				std::string type; // buy or sell   buy   exchangePair:purchaseAsset/payAsset   sell: exchangePair:payAsset/purchaseAsset
				uint64_t expiredAt; //时间值是指从1970-01-01 00:00:00 +0000 (UTC) 这个时间点开始到当前时间的秒数
				uint64_t version;
			};
			struct Order {
				std::string orderInfo;
				std::string sig;
				std::string id;
			};
			struct FillOrder {
				exchange::Order order;
				int64_t getNum;
				int64_t spentNum;
			};
			struct MatchInfo {
				FillOrder fillTakerOrder;
				std::vector<FillOrder> fillMakerOrders;
			};
		}

		// this is native contract for exchange    //match by relayer off chain, replay matched orders on chain
		class exchange_native_contract : public uvm::contract::abstract_native_contract_impl
		{
		private:
			std::shared_ptr<uvm::contract::native_contract_interface> _proxy;
			void checkMatchedOrders(exchange::FillOrder& takerFillOrder, std::vector<exchange::FillOrder>& makerFillOrders);
			exchange::OrderInfo checkOrder(const exchange::FillOrder& fillOrder, std::string& addr, std::string& id, std::string& eventOrder,bool& isCompleted);
		public:
			static std::string native_contract_key() { return "exchange"; }

			exchange_native_contract(std::shared_ptr<uvm::contract::native_contract_interface> proxy) : _proxy(proxy) {}
			virtual ~exchange_native_contract() {}

			virtual std::shared_ptr<uvm::contract::native_contract_interface> get_proxy() const { return _proxy; }

			virtual std::string contract_key() const;
			virtual std::set<std::string> apis() const;
			virtual std::set<std::string> offline_apis() const;
			virtual std::set<std::string> events() const;

			virtual void invoke(const std::string& api_name, const std::string& api_arg);
			std::string check_admin();
			std::string get_storage_state();

			std::string get_from_address();

			void init_api(const std::string& api_name, const std::string& api_arg);
			void init_config_api(const std::string& api_name, const std::string& api_arg);

			void state_api(const std::string& api_name, const std::string& api_arg);
			void minFee_api(const std::string& api_name, const std::string& api_arg);

			void fillOrder_api(const std::string& api_name, const std::string& api_arg);
			void setMinFee_api(const std::string& api_name, const std::string& api_arg);

			void on_deposit_asset_api(const std::string& api_name, const std::string& api_args);
			void withdraw_api(const std::string& api_name, const std::string& api_arg);

			void balanceOf_api(const std::string& api_name, const std::string& api_arg);
			void getOrder_api(const std::string& api_name, const std::string& api_arg);
			void cancelOrders_api(const std::string& api_name, const std::string& api_args);

			void getAddrByPubk_api(const std::string& api_name, const std::string& api_arg);
			void balanceOfPubk_api(const std::string& api_name, const std::string& api_arg);

		};

	}
}

namespace fc {
	/*void to_variant(const simplechain::asset& var, variant& vo) {
	fc::mutable_variant_object obj("asset_id", var.asset_id);
	obj("symbol", var.symbol);
	obj("precision", var.precision);
	vo = std::move(obj);
	}*/
	void from_variant(const variant& var, uvm::contract::exchange::OrderInfo& vo);
	void from_variant(const variant& var, uvm::contract::exchange::Order& vo);
	void from_variant(const variant& var, uvm::contract::exchange::FillOrder& vo);
	void from_variant(const variant& var, uvm::contract::exchange::MatchInfo& vo);
}
