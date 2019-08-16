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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS ORcandidate_create_operation
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain { 

  /**
    * @brief Create a candidate object, as a bid to hold a candidate position on the network.
    * @ingroup operations
    *
    * Accounts which wish to become candidate may use this operation to create a candidate object which stakeholders may
    * vote on to approve its position as a candidate.
    */
   struct candidate_create_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 10000 * GRAPHENE_XWCCHAIN_PRECISION; };

      asset             fee;
      /// The account which owns the candidate. This account pays the fee for this operation.
	  address           candidate_address;
      account_id_type   candidate_account;
      string            url;
      public_key_type   block_signing_key;
	  optional<guarantee_object_id_type> guarantee_id;
	  optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
      address fee_payer()const { return candidate_address; }
      void            validate()const;
	  void get_required_authorities(vector<authority>& a)const {
		  a.push_back(authority(1, candidate_address, 1));
	  }
   };

  /**
    * @brief Update a witness object's URL and block signing key.
    * @ingroup operations
    */
   struct witness_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         share_type fee = 0.001 * GRAPHENE_XWCCHAIN_PRECISION;
      };

      asset             fee;
      /// The witness object to update.
      candidate_id_type   witness;
      /// The account which owns the witness. This account pays the fee for this operation.
      account_id_type   witness_account;
      /// The new URL.
      optional< string > new_url;
      /// The new block signing key.
      optional< public_key_type > new_signing_key;

      account_id_type fee_payer()const { return witness_account; }
      void            validate()const;
   };

   //candidate to generate new multi-address for specific asset
   struct candidate_generate_multi_asset_operation :public base_operation
   {
	   struct fee_parameters_type
	   {
		   share_type fee = share_type(0);
	   };
	   asset fee;
	   candidate_id_type candidate;
	   string chain_type;
	   address candidate_address;
	   string multi_address_hot;
	   string multi_redeemScript_hot;
	   string multi_address_cold;
	   string multi_redeemScript_cold;
	   address fee_payer() const { return candidate_address;} 
	   void validate() const;
	   void get_required_authorities(vector<authority>& a)const {
		   a.push_back(authority(1, candidate_address, 1));
	   }
	   share_type calculate_fee(const fee_parameters_type& k)const { return 0; }
   };

   struct candidate_merge_signatures_operation :public base_operation
   {
	   struct fee_parameters_type
	   {
		   share_type fee = 0 * GRAPHENE_BLOCKCHAIN_PRECISION;
	   };
	   asset fee;
	   candidate_id_type candidate;
	   string chain_type;
	   address candidate_address;
	   multisig_asset_transfer_id_type id;
	   address fee_payer() const { return candidate_address; }
	   void validate() const;
	   void get_required_authorities(vector<authority>& a)const {
		   a.push_back(authority(1, candidate_address, 1));
	   }
	   share_type calculate_fee(const fee_parameters_type& k)const { return 0; }
   };

   /// TODO: witness_resign_operation : public base_operation

} } // graphene::chain

FC_REFLECT( graphene::chain::candidate_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::candidate_create_operation, (fee)(candidate_address)(candidate_account)(url)(guarantee_id)(block_signing_key) )

FC_REFLECT( graphene::chain::witness_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::witness_update_operation, (fee)(witness)(witness_account)(new_url)(new_signing_key) )

FC_REFLECT(graphene::chain::candidate_generate_multi_asset_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::candidate_generate_multi_asset_operation, (fee)(candidate)(chain_type)(candidate_address)(multi_address_hot)(multi_redeemScript_hot)(multi_address_cold)(multi_redeemScript_cold))

FC_REFLECT(graphene::chain::candidate_merge_signatures_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::candidate_merge_signatures_operation, (fee)(candidate)(chain_type)(candidate_address)(id))
