/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <algorithm>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/smart_ref_impl.hpp>
#include <iostream>
namespace fc
{
   // explicitly instantiate the smart_ref, gcc fails to instantiate it in some release builds
   //template graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator=(smart_ref<graphene::chain::fee_schedule>&&);
   //template graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator=(U&&);
   //template graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator=(const smart_ref&);
   //template smart_ref<graphene::chain::fee_schedule>::smart_ref();
   //template const graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator*() const;
}

#define MAX_FEE_STABILIZATION_ITERATION 4

namespace graphene { namespace chain {

   typedef fc::smart_ref<fee_schedule> smart_fee_schedule;

   static smart_fee_schedule tmp;

   fee_schedule::fee_schedule()
   {
   }

   fee_schedule fee_schedule::get_default()
   {
      fee_schedule result;
      for( int i = 0; i < fee_parameters().count(); ++i )
      {
         fee_parameters x; x.set_which(i);
         result.parameters.insert(x);
      }
      return result;
   }

   struct fee_schedule_validate_visitor
   {
      typedef void result_type;

      template<typename T>
      void operator()( const T& p )const
      {
         //p.validate();
      }
   };

   void fee_schedule::validate()const
   {
      for( const auto& f : parameters )
         f.visit( fee_schedule_validate_visitor() );
   }

   struct calc_fee_visitor
   {
      typedef uint64_t result_type;

      const fee_parameters& param;
      calc_fee_visitor( const fee_parameters& p ):param(p){}

      template<typename OpType>
      result_type operator()(  const OpType& op )const
      {
         return op.calculate_fee( param.get<typename OpType::fee_parameters_type>() ).value;
      }
   };

   struct set_fee_visitor
   {
      typedef void result_type;
      asset _fee;

      set_fee_visitor( asset f ):_fee(f){}

      template<typename OpType>
      void operator()( OpType& op )const
      {
         op.fee = _fee;
      }
   };

   struct zero_fee_visitor
   {
      typedef void result_type;

      template<typename ParamType>
      result_type operator()(  ParamType& op )const
      {
         memset( (char*)&op, 0, sizeof(op) );
      }
   };

   void fee_schedule::zero_all_fees()
   {
      *this = get_default();
      for( fee_parameters& i : parameters )
         i.visit( zero_fee_visitor() );
      this->scale = 0;
   }

   asset fee_schedule::calculate_fee( const operation& op, const price& core_exchange_rate )const
   {
      //idump( (op)(core_exchange_rate) );
      fee_parameters params; params.set_which(op.which());
      auto itr = parameters.find(params);
      if( itr != parameters.end() ) params = *itr;
      auto base_value = op.visit( calc_fee_visitor( params ) );
      auto scaled = fc::uint128(base_value) * scale;
      scaled /= GRAPHENE_100_PERCENT;
      FC_ASSERT( scaled <= GRAPHENE_MAX_SHARE_SUPPLY );
      //idump( (base_value)(scaled)(core_exchange_rate) );
      auto result = asset( scaled.to_uint64(), asset_id_type(0) ) * core_exchange_rate;
      //FC_ASSERT( result * core_exchange_rate >= asset( scaled.to_uint64()) );

      while( result * core_exchange_rate < asset( scaled.to_uint64()) )
        result.amount++;

      FC_ASSERT( result.amount <= GRAPHENE_MAX_SHARE_SUPPLY );
      return result;
   }

   asset fee_schedule::set_fee( operation& op, const price& core_exchange_rate )const
   {
      auto f = calculate_fee( op, core_exchange_rate );
      auto f_max = f;
      for( int i=0; i<MAX_FEE_STABILIZATION_ITERATION; i++ )
      {
         op.visit( set_fee_visitor( f_max ) );
         auto f2 = calculate_fee( op, core_exchange_rate );
         if( f == f2 )
            break;
         f_max = std::max( f_max, f2 );
         f = f2;
         if( i == 0 )
         {
            // no need for warnings on later iterations
            wlog( "set_fee requires multiple iterations to stabilize with core_exchange_rate ${p} on operation ${op}",
               ("p", core_exchange_rate) ("op", op) );
         }
      }
      return f_max;
   }

   chain_parameters::chain_parameters() {
   }

   // copy constructor
   chain_parameters::chain_parameters(const chain_parameters& other)
   {
      current_fees = other.current_fees;
      safe_copy(*this, other);
   }

   // copies the easy stuff
   void chain_parameters::safe_copy(chain_parameters& to, const chain_parameters& from)
   {
      to.block_interval                         = from.block_interval;
      to.maintenance_interval                   = from.maintenance_interval;
      to.maintenance_skip_slots                 = from.maintenance_skip_slots;
      to.committee_proposal_review_period       = from.committee_proposal_review_period;
      to.maximum_transaction_size               = from.maximum_transaction_size;
      to.maximum_block_size                     = from.maximum_block_size;
      to.maximum_time_until_expiration          = from.maximum_time_until_expiration;
      to.maximum_proposal_lifetime              = from.maximum_proposal_lifetime;
      to.maximum_asset_whitelist_authorities    = from.maximum_asset_whitelist_authorities;
      to.maximum_asset_feed_publishers          = from.maximum_asset_feed_publishers;
      to.maximum_miner_count                    = from.maximum_miner_count;
      to.maximum_wallfacer_count                = from.maximum_wallfacer_count;
      to.minimum_wallfacer_count                = from.minimum_wallfacer_count; 
      to.maximum_authority_membership           = from.maximum_authority_membership;
      to.reserve_percent_of_fee                 = from.reserve_percent_of_fee;
      to.network_percent_of_fee                 = from.network_percent_of_fee;
      to.lifetime_referrer_percent_of_fee       = from.lifetime_referrer_percent_of_fee;
      to.cashback_vesting_period_seconds        = from.cashback_vesting_period_seconds;
      to.cashback_vesting_threshold             = from.cashback_vesting_threshold;
      to.count_non_member_votes                 = from.count_non_member_votes;
      to.allow_non_member_whitelists            = from.allow_non_member_whitelists;
      to.miner_pay_per_block                    = from.miner_pay_per_block;
      to.miner_pay_vesting_seconds              = from.miner_pay_vesting_seconds;
      to.worker_budget_per_day                  = from.worker_budget_per_day;
      to.max_predicate_opcode                   = from.max_predicate_opcode;
      to.fee_liquidation_threshold              = from.fee_liquidation_threshold;
      to.accounts_per_fee_scale                 = from.accounts_per_fee_scale;
      to.account_fee_scale_bitshifts            = from.account_fee_scale_bitshifts;
      to.max_authority_depth                    = from.max_authority_depth;
      to.minimum_pledge_weight_line             = from.minimum_pledge_weight_line;
      to.minimum_wallfacer_pledge_line          = from.minimum_wallfacer_pledge_line;
      to.bonus_distribute_limit                 = from.bonus_distribute_limit;
      to.min_pay_back_balance                   = from.min_pay_back_balance;
      to.min_pay_back_balance_other_asset       = from.min_pay_back_balance_other_asset;
      to.validate_time_period                   = from.validate_time_period;
      to.extensions                             = from.extensions;
   }
   
   void chain_parameters::validate()const
   {
      current_fees->validate();
      FC_ASSERT( reserve_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( network_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( lifetime_referrer_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( network_percent_of_fee + lifetime_referrer_percent_of_fee <= GRAPHENE_100_PERCENT );

      FC_ASSERT( block_interval >= GRAPHENE_MIN_BLOCK_INTERVAL );
      FC_ASSERT( block_interval <= GRAPHENE_MAX_BLOCK_INTERVAL );
      FC_ASSERT( block_interval > 0 );
      FC_ASSERT( maintenance_interval > block_interval,
                 "Maintenance interval must be longer than block interval" );
      FC_ASSERT( maintenance_interval % block_interval == 0,
                 "Maintenance interval must be a multiple of block interval" );
      FC_ASSERT( maximum_transaction_size >= GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT,
                 "Transaction size limit is too low" );
      FC_ASSERT( maximum_block_size >= GRAPHENE_MIN_BLOCK_SIZE_LIMIT,
                 "Block size limit is too low" );
      FC_ASSERT( maximum_time_until_expiration > block_interval,
                 "Maximum transaction expiration time must be greater than a block interval" );
      FC_ASSERT( maximum_proposal_lifetime - committee_proposal_review_period > block_interval,
                 "Committee proposal review period must be less than the maximum proposal lifetime" );
   }

} } // graphene::chain
