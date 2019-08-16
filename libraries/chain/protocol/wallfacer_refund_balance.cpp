#include <graphene/chain/protocol/wallfacer_refund_balance.hpp>
#include <graphene/chain/database.hpp>
namespace graphene {
	namespace chain {
		void wallfacer_refund_balance_operation::validate() const
		{
			FC_ASSERT(fee.amount >= 0);
		}
	}
}
