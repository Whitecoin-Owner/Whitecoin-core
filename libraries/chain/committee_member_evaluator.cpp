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
#include <graphene/chain/committee_member_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/referendum_object.hpp>
#include <graphene/chain/lockbalance_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/wallfacer_lock_balance_object.hpp>

#include <fc/smart_ref_impl.hpp>

#define GUARD_VOTES_EXPIRATION_TIME 7*24*3600
#define  MINER_VOTES_REVIWE_TIME  24 * 3600
namespace graphene {
    namespace chain {

        void_result wallfacer_member_create_evaluator::do_evaluate(const wallfacer_member_create_operation& op)
        {
            try {
                //FC_ASSERT(db().get(op.wallfacer_member_account).is_lifetime_member());
                 // account cannot be a candidate
                auto& iter = db().get_index_type<candidate_index>().indices().get<by_account>();
                FC_ASSERT(iter.find(op.wallfacer_member_account) == iter.end(), "account cannot be a candidate.");       
				FC_ASSERT(db().get(op.wallfacer_member_account).addr == op.fee_pay_address);
                return void_result();
            } FC_CAPTURE_AND_RETHROW((op))
        }

        object_id_type wallfacer_member_create_evaluator::do_apply(const wallfacer_member_create_operation& op)
        {
            try {
                vote_id_type vote_id;
                db().modify(db().get_global_properties(), [&vote_id](global_property_object& p) {
                    vote_id = get_next_vote_id(p, vote_id_type::committee);
                });
                const auto& new_del_object = db().create<wallfacer_member_object>([&](wallfacer_member_object& obj) {
                    obj.wallfacer_member_account = op.wallfacer_member_account;
                    obj.vote_id = vote_id;
					obj.formal = false;
                });
                return new_del_object.id;
            } FC_CAPTURE_AND_RETHROW((op))
        }
		void wallfacer_member_create_evaluator::pay_fee()
		{
			FC_ASSERT(core_fees_paid.asset_id == asset_id_type());
			db().modify(db().get(asset_id_type()).dynamic_asset_data_id(db()), [this](asset_dynamic_data_object& d) {
				d.current_supply -= this->core_fees_paid.amount;
			});
		}
        void_result wallfacer_member_update_evaluator::do_evaluate(const wallfacer_member_update_operation& op)
        {
            try {
				auto replace_queue = op.replace_queue;
				const auto& referendum_idx = db().get_index_type<referendum_index>().indices().get<by_id>();
				for (const auto& referendum : referendum_idx)
				{
					for (const auto& op_r : referendum.proposed_transaction.operations)
					{

						FC_ASSERT(op_r.which() != operation::tag<candidate_referendum_wallfacer_operation>::value, "there is other referendum for wallfacer election.");
					}
				}
				const auto&  wallfacer_idx = db().get_index_type<wallfacer_member_index>().indices().get<by_account>();
				FC_ASSERT(op.replace_queue.size() >0 && op.replace_queue.size() <= 1);
				fc::flat_set<account_id_type> target;
				for (const auto& itr : replace_queue)
				{
					auto itr_first = wallfacer_idx.find(itr.first);
					FC_ASSERT(itr_first != wallfacer_idx.end() && itr_first->formal != true);
					auto itr_second = wallfacer_idx.find(itr.second);
					FC_ASSERT(itr_second != wallfacer_idx.end() && itr_second->formal == true && itr_second->wallfacer_type== PERMANENT);
					target.insert(itr.second);
				}
				FC_ASSERT(target.size() == replace_queue.size(), "replaced wallfacers should be unique.");
                return void_result();
            } FC_CAPTURE_AND_RETHROW((op))
        }

        void_result wallfacer_member_update_evaluator::do_apply(const wallfacer_member_update_operation& op)
        {
            try {
                database& _db = db();
				for (const auto& itr : op.replace_queue)
				{
					_db.modify(
						*_db.get_index_type<wallfacer_member_index>().indices().get<by_account>().find(itr.first),
						[&](wallfacer_member_object& com)
					{
						com.wallfacer_type = PERMANENT;
						com.formal = true;
					});

					_db.modify(
						*_db.get_index_type<wallfacer_member_index>().indices().get<by_account>().find(itr.second),
						[&](wallfacer_member_object& com)
					{
						com.wallfacer_type = PERMANENT;
						com.formal = false;
					});
					
				}
                return void_result();
            } FC_CAPTURE_AND_RETHROW((op))
        }

        void_result committee_member_update_global_parameters_evaluator::do_evaluate(const committee_member_update_global_parameters_operation& o)
        {
            try {
				FC_ASSERT(trx_state->_is_proposed_trx);
				return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }

        void_result committee_member_update_global_parameters_evaluator::do_apply(const committee_member_update_global_parameters_operation& o)
        {
            try {
                db().modify(db().get_global_properties(), [&o](global_property_object& p) {
                    p.pending_parameters = o.new_parameters;
                });

                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }


        void_result committee_member_execute_coin_destory_operation_evaluator::do_evaluate(const committee_member_execute_coin_destory_operation& o)
        {
            try {
                FC_ASSERT(trx_state->_is_proposed_trx);

                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }
        void_result committee_member_execute_coin_destory_operation_evaluator::do_apply(const committee_member_execute_coin_destory_operation& o)
        {
            try {
                //执行退币流程
                database& _db = db();
                //执行candidate质押退币流程
                const auto& lock_balances = _db.get_index_type<lockbalance_index>().indices();
                const auto& wallfacer_lock_balances = _db.get_index_type<wallfacer_lock_balance_index>().indices();
                const auto& all_wallfacer = _db.get_index_type<wallfacer_member_index>().indices();
                const auto& all_candidate = _db.get_index_type<candidate_index>().indices();
                share_type loss_money = o.loss_asset.amount;
                share_type candidate_need_pay_money = loss_money * o.commitee_member_handle_percent / 100;
                share_type wallfacer_need_pay_money = 0;
                share_type Total_money = 0;
                share_type Total_pay = 0;
                auto asset_obj = _db.get(o.loss_asset.asset_id);
                auto asset_symbol = asset_obj.symbol;
                if (o.commitee_member_handle_percent != 100) {
                    for (auto& one_balance : lock_balances) {
                        if (one_balance.lock_asset_id == o.loss_asset.asset_id) {
                            Total_money += one_balance.lock_asset_amount;
                            //按比例扣除
                        }
                    }
                    double percent = (double)candidate_need_pay_money.value / (double)Total_money.value;
                    for (auto& one_balance : lock_balances) {
                        if (one_balance.lock_asset_id == o.loss_asset.asset_id) {
                            share_type pay_amount = (uint64_t)(one_balance.lock_asset_amount.value*percent);
                            _db.modify(one_balance, [&](lockbalance_object& obj) {
                                obj.lock_asset_amount -= pay_amount;
                            });
                            _db.modify(_db.get(one_balance.lockto_candidate_account), [&](candidate_object& obj) {
                                if (obj.lockbalance_total.count(asset_symbol)) {
                                    obj.lockbalance_total[asset_symbol] -= pay_amount;
                                }

                            });
                            Total_pay += pay_amount;
                            //按比例扣除
                        }
                    }
                }

                if (o.commitee_member_handle_percent != 0)
                {
                    wallfacer_need_pay_money = loss_money - Total_pay;
                    int count = all_wallfacer.size();
                    share_type one_wallfacer_pay_money = wallfacer_need_pay_money.value / count;
                    share_type remaining_amount = wallfacer_need_pay_money - one_wallfacer_pay_money* count;
                    bool first_flag = true;
                    for (auto& temp_lock_balance : wallfacer_lock_balances)
                    {

                        if (temp_lock_balance.lock_asset_id == o.loss_asset.asset_id)
                        {
                            share_type pay_amount;
                            if (first_flag)
                            {
                                pay_amount = one_wallfacer_pay_money + remaining_amount;
                                first_flag = false;
                            }
                            else
                            {
                                pay_amount = one_wallfacer_pay_money;
                            }

                            _db.modify(temp_lock_balance, [&](wallfacer_lock_balance_object& obj) {
                                obj.lock_asset_amount -= pay_amount;

                            });

                            _db.modify(_db.get(temp_lock_balance.lock_balance_account), [&](wallfacer_member_object& obj) {

                                obj.wallfacer_lock_balance[asset_symbol] -= pay_amount;

                            });

                            Total_pay += pay_amount;
                            //按比例扣除
                        }
                    }

                }
                FC_ASSERT(Total_pay == o.loss_asset.amount);

                //执行跨链修改代理流程
                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }

        void_result chain::wallfacer_member_resign_evaluator::do_evaluate(const wallfacer_member_resign_operation & o)
        {
            try {
                auto &wallfacers = db().get_index_type<wallfacer_member_index>().indices().get<by_id>();
                FC_ASSERT(wallfacers.find(o.wallfacer_member_account) == wallfacers.end(), "No referred wallfacer is found, must be invalid resign operation.");
                auto num = 0;
                std::for_each(wallfacers.begin(), wallfacers.end(), [&num](const wallfacer_member_object &g) {
                    if (g.formal) {
                        ++num;
                    }
                });
                FC_ASSERT(num > db().get_global_properties().parameters.minimum_wallfacer_count, "No enough wallfacers.");
                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }

        object_id_type chain::wallfacer_member_resign_evaluator::do_apply(const wallfacer_member_resign_operation & o)
        {
            try {
                auto &iter = db().get_index_type<wallfacer_member_index>().indices().get<by_account>();
                auto wallfacer = iter.find(o.wallfacer_member_account);
                db().modify(*wallfacer, [](wallfacer_member_object& g) {
                    g.formal = false;
                });
                return object_id_type(o.wallfacer_member_account.space_id, o.wallfacer_member_account.type_id, o.wallfacer_member_account.instance.value);
            } FC_CAPTURE_AND_RETHROW((o))
        }

		void_result candidate_referendum_wallfacer_evaluator::do_evaluate(const candidate_referendum_wallfacer_operation& o)
		{
			try {
				const auto& _db = db();
				const auto& all_wallfacer_ic = _db.get_index_type<wallfacer_member_index>().indices().get<by_account>();
				const auto& all_candidate_ic = _db.get_index_type<candidate_index>().indices().get<by_account>();
				const auto& referendum_idx = db().get_index_type<referendum_index>().indices().get<by_pledge>();
				auto itr = referendum_idx.rbegin();
				if (itr != referendum_idx.rend())
					_id = itr->id;
				FC_ASSERT(o.replace_queue.size() > 0 && o.replace_queue.size() <=3 );
				fc::flat_set<account_id_type> target;
				for (const auto& iter : o.replace_queue)
				{
					auto itr_first_wallfacer = all_wallfacer_ic.find(iter.first);
					auto itr_first_candidate = all_candidate_ic.find(iter.first);
					auto itr_second_wallfacer = all_wallfacer_ic.find(iter.second);
					FC_ASSERT(itr_second_wallfacer != all_wallfacer_ic.end() && itr_second_wallfacer->formal == true && itr_second_wallfacer->wallfacer_type != PERMANENT);
					FC_ASSERT(itr_first_candidate == all_candidate_ic.end() && (itr_first_wallfacer != all_wallfacer_ic.end() && itr_first_wallfacer->formal != true));
					target.insert(iter.second);
				}
				FC_ASSERT(o.replace_queue.size() == target.size(),"replaced wallfacer should be unique.");
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result candidate_referendum_wallfacer_evaluator::do_apply(const candidate_referendum_wallfacer_operation& o)
		{
			try {
				auto& _db = db();
				auto& all_wallfacer_ic = _db.get_index_type<wallfacer_member_index>().indices().get<by_account>();
				auto& all_candidate_ic = _db.get_index_type<candidate_index>().indices().get<by_account>();
				for (const auto& iter : o.replace_queue)
				{
					auto itr_first_wallfacer = all_wallfacer_ic.find(iter.first);
					auto itr_first_candidate = all_candidate_ic.find(iter.first);
					auto itr_second_wallfacer = all_wallfacer_ic.find(iter.second);
					//FC_ASSERT(itr_second_wallfacer != all_wallfacer_ic.end() && itr_second_wallfacer->formal == true);
					//FC_ASSERT(itr_first_candidate == all_candidate_ic.end() && (itr_first_wallfacer == all_wallfacer_ic.end() || (itr_first_wallfacer != all_wallfacer_ic.end() && itr_first_wallfacer->formal != true)));
					_db.modify(*itr_second_wallfacer, [](wallfacer_member_object& obj) {
						obj.formal = false;
					});
					if (itr_first_wallfacer == all_wallfacer_ic.end())
					{
						vote_id_type vote_id;
						db().modify(db().get_global_properties(), [&vote_id](global_property_object& p) {
							vote_id = get_next_vote_id(p, vote_id_type::committee);
						});
						_db.create<wallfacer_member_object>([&] (wallfacer_member_object& obj){
							obj.wallfacer_member_account = iter.first;
							obj.vote_id = vote_id;
							obj.url = "";
							obj.formal = true;
							obj.wallfacer_type = EXTERNAL;
							obj.which_id = fc::variant(_id).as_string();
						});
					}
					else
					{
						_db.modify(*itr_first_wallfacer, [&] (wallfacer_member_object& obj){
							obj.formal = true;
							obj.wallfacer_type = EXTERNAL;
							obj.which_id = fc::variant(_id).as_string();
						});
					}
				}
				return void_result();

			}FC_CAPTURE_AND_RETHROW((o))
		}
		bool candidate_referendum_wallfacer_evaluator::if_evluate()
		{
			return true;
		}
    }
} // graphene::chain
