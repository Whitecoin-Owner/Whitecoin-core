

#include <graphene/crosschain_privatekey_management/private_key.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/optional.hpp>
#include <graphene/chain/pts_address.hpp>
#include <bitcoin/bitcoin.hpp>
#include <graphene/crosschain_privatekey_management/util.hpp>
#include <assert.h>
#include <graphene/utilities/hash.hpp>
#include <list>
#include <libdevcore/DevCoreCommonJS.h>
#include <libdevcore/RLP.h>
#include <libdevcore/FixedHash.h>
#include <libethcore/TransactionBase.h>
namespace graphene { namespace privatekey_management {
	std::string BinToHex(const std::vector<char> &strBin, bool bIsUpper)
	{
		std::string strHex;
		strHex.resize(strBin.size() * 2);
		for (size_t i = 0; i < strBin.size(); i++)
		{
			uint8_t cTemp = strBin[i];
			for (size_t j = 0; j < 2; j++)
			{
				uint8_t cCur = (cTemp & 0x0f);
				if (cCur < 10)
				{
					cCur += '0';
				}
				else
				{
					cCur += ((bIsUpper ? 'A' : 'a') - 10);
				}
				strHex[2 * i + 1 - j] = cCur;
				cTemp >>= 4;
			}
		}

		return strHex;
	}


	crosschain_privatekey_base::crosschain_privatekey_base()
	{
		_key = fc::ecc::private_key();
	}

	crosschain_privatekey_base::crosschain_privatekey_base(fc::ecc::private_key& priv_key)
	{
		_key = priv_key;
	}

	fc::ecc::private_key  crosschain_privatekey_base::get_private_key()
	{
		FC_ASSERT(this->_key != fc::ecc::private_key(), "private key is empty!");
		
		return _key;
	}

	std::string  crosschain_privatekey_base::sign_trx(const std::string& raw_trx,int index)
	{
		//get endorsement
		libbitcoin::endorsement out;
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		auto prev_script = libbitcoin::chain::script::to_pay_key_hash_pattern(libbitcoin_priv.to_payment_address().hash());
		libbitcoin::chain::script   libbitcoin_script;//(libbitcoin::data_chunk(base),true);	
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(raw_trx));
		uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;

		auto result = libbitcoin::chain::script::create_endorsement(out, libbitcoin_priv.secret(), prev_script, trx, index, hash_type);
		assert( result == true);
// 		printf("endorsement is %s\n", libbitcoin::encode_base16(out).c_str());

		//get public hex
		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();
		//get signed raw-trx
		std::string endorsment_script = "[" + libbitcoin::encode_base16(out) + "]" + " [" + pub_hex + "] ";
// 		printf("endorsement script is %s\n", endorsment_script.c_str());
		libbitcoin_script.from_string(endorsment_script);

		//trx.from_data(libbitcoin::config::base16(raw_trx));
		trx.inputs()[index].set_script(libbitcoin_script);	    
		std::string signed_trx = libbitcoin::encode_base16(trx.to_data());
// 		printf("signed trx is %s\n", signed_trx.c_str());
		return signed_trx;
	}

	void crosschain_privatekey_base::generate(fc::optional<fc::ecc::private_key> k)
	{
		if (!k.valid())
		{

			_key = fc::ecc::private_key::generate();
		}
		else
		{
			_key = *k;
		}
	}

	std::string crosschain_privatekey_base::sign_message(const std::string& msg)
	{


		libbitcoin::wallet::message_signature sign;

		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
// 		libbitcoin::wallet::ec_private libbitcoin_priv("L13gvvM3TtL2EmfBdye8tp4tQhcbCG3xz3VPrBjSZL8MeJavLL8K");
		libbitcoin::data_chunk  data(msg.begin(), msg.end());

		libbitcoin::wallet::sign_message(sign, data, libbitcoin_priv.secret());

		auto result = libbitcoin::encode_base64(sign);
// 		printf("the signed message is %s\n", result.c_str());
		return result;

	}
	bool crosschain_privatekey_base::validate_address(const std::string& addr)
	{
		try {
			graphene::chain::pts_address pts(addr);
			return pts.is_valid() && (pts.version() == get_pubkey_prefix()|| pts.version() == get_script_prefix());
		}
		catch (fc::exception& e) {
			return false;
		}
		
	}
	bool crosschain_privatekey_base::validate_transaction(const std::string& addr,const std::string& redeemscript,const std::string& sig)
	{
		return graphene::utxo::validateUtxoTransaction(addr,redeemscript,sig);
	}
	fc::variant_object crosschain_privatekey_base::combine_trxs(const std::vector<std::string>& trxs)
	{
		auto trx = graphene::utxo::combine_trx(trxs);
		fc::mutable_variant_object result;
		result["trx"]=fc::json::from_string(graphene::utxo::decoderawtransaction(trx,get_pubkey_prefix(),get_script_prefix())).get_object();
		result["hex"] = trx;
		return result;
	}

	bool crosschain_privatekey_base::verify_message(const std::string addr, const std::string& content, const std::string& encript)
	{
		return true;
	}

	void btc_privatekey::init()
	{
		set_id(0);
		//set_pubkey_prefix(0x6F);
		//set_script_prefix(0xC4);
		//set_privkey_prefix(0xEF);
		set_pubkey_prefix(btc_pubkey);
		set_script_prefix(btc_script);
		set_privkey_prefix(btc_privkey);
	}



	std::string  btc_privatekey::get_wif_key()
	{	
		FC_ASSERT( is_empty() == false, "private key is empty!" );

		fc::sha256 secret = get_private_key().get_secret();
		//one byte for prefix, one byte for compressed sentinel
		const size_t size_of_data_to_hash = sizeof(secret) + 2;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)get_privkey_prefix();
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		data[size_of_data_to_hash - 1] = (char)0x01;
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));
	
	}

    std::string   btc_privatekey::get_address()
    {
		FC_ASSERT(is_empty() == false, "private key is empty!");

        //configure for bitcoin
        uint8_t version = get_pubkey_prefix();
        bool compress = true;

		const fc::ecc::private_key& priv_key = get_private_key();
        fc::ecc::public_key  pub_key = priv_key.get_public_key();

        graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
    }
	std::string btc_privatekey::get_address_by_pubkey(const std::string& pub)
	{
		return graphene::privatekey_management::get_address_by_pubkey(pub, get_pubkey_prefix());
	}
	std::string btc_privatekey::get_public_key()
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());

		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();

		return pub_hex;

	}

	std::string btc_privatekey::sign_message(const std::string& msg)
	{
		return this->crosschain_privatekey_base::sign_message(msg);
	}
	std::string btc_privatekey::sign_trx(const std::string& raw_trx,int index)
	{
		return this->crosschain_privatekey_base::sign_trx(raw_trx,index);
	}

	fc::optional<fc::ecc::private_key>   btc_privatekey::import_private_key(const std::string& wif_key)
	{
		auto key = graphene::utilities::wif_to_key(wif_key);
		set_key(*key);
		return key;

	}
	std::string btc_privatekey::mutisign_trx( const std::string& redeemscript, const fc::variant_object& raw_trx)
	{
		try {
			FC_ASSERT(raw_trx.contains("hex"));
			FC_ASSERT(raw_trx.contains("trx"));
			auto tx = raw_trx["trx"].get_object();
			auto size = tx["vin"].get_array().size();
			std::string trx = raw_trx["hex"].as_string();
			for (int index = 0; index < size; index++)
			{
				auto endorse = graphene::privatekey_management::create_endorsement(get_wif_key(), redeemscript,trx,index);
				trx = graphene::privatekey_management::mutisign_trx(endorse,redeemscript,trx,index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));
	}



	void ltc_privatekey::init()
	{
		set_id(0);
		//set_pubkey_prefix(0x6F);
		//set_script_prefix(0x3A);
		//set_privkey_prefix(0xEF);
		set_pubkey_prefix(ltc_pubkey);
		set_script_prefix(ltc_script);
		set_privkey_prefix(ltc_privkey);
	}

	std::string  ltc_privatekey::get_wif_key()
	{
		/*fc::sha256& secret = priv_key.get_secret();

		const size_t size_of_data_to_hash = sizeof(secret) + 1 ;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes + 1];
		data[0] = (char)0xB0;
		memcpy(&data[1], (char*)&secret, sizeof(secret));

		// add compressed byte
		char value = (char)0x01;
		memcpy(data + size_of_data_to_hash, (char *)&value, 1);
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash + 1, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));*/

		FC_ASSERT(is_empty() == false, "private key is empty!");

		const fc::ecc::private_key& priv_key = get_private_key();
		const fc::sha256& secret = priv_key.get_secret();

		const size_t size_of_data_to_hash = sizeof(secret) + 2;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)get_privkey_prefix();
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		data[size_of_data_to_hash - 1] = (char)0x01;
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));


	}



	std::string ltc_privatekey::get_address()
	{
		FC_ASSERT(is_empty() == false, "private key is empty!");

		//configure for bitcoin
		uint8_t version = get_pubkey_prefix();
		bool compress = true;

		const fc::ecc::private_key& priv_key = get_private_key();
		fc::ecc::public_key  pub_key = priv_key.get_public_key();

		graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
	}

	std::string ltc_privatekey::sign_message(const std::string& msg)
	{
		libbitcoin::wallet::message_signature sign;

		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		// 		libbitcoin::wallet::ec_private libbitcoin_priv("L13gvvM3TtL2EmfBdye8tp4tQhcbCG3xz3VPrBjSZL8MeJavLL8K");
		libbitcoin::data_chunk  data(msg.begin(), msg.end());
		
		libbitcoin::wallet::sign_message(sign, data, libbitcoin_priv.secret(),true, std::string("Litecoin Signed Message:\n"));

		auto result = libbitcoin::encode_base64(sign);
		// 		printf("the signed message is %s\n", result.c_str());
		return result;
	}

	bool ltc_privatekey::verify_message(const std::string addr, const std::string& content, const std::string& encript)
	{
		return graphene::utxo::verify_message(addr, content, encript, "Litecoin Signed Message:\n");
	}

	std::string ltc_privatekey::mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx)
	{
		try {
			FC_ASSERT(raw_trx.contains("hex"));
			FC_ASSERT(raw_trx.contains("trx"));
			auto tx = raw_trx["trx"].get_object();
			auto size = tx["vin"].get_array().size();
			std::string trx = raw_trx["hex"].as_string();
			for (int index = 0; index < size; index++)
			{
				auto endorse = graphene::privatekey_management::create_endorsement(get_wif_key(), redeemscript, trx, index);
				trx = graphene::privatekey_management::mutisign_trx(endorse, redeemscript, trx, index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));



	}

	std::string ltc_privatekey::get_public_key()
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());

		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();

		return pub_hex;

	}
	std::string ltc_privatekey::get_address_by_pubkey(const std::string& pub)
	{
		return graphene::privatekey_management::get_address_by_pubkey(pub, get_pubkey_prefix());
	}
	fc::optional<fc::ecc::private_key> ltc_privatekey::import_private_key(const std::string& wif_key)
	{
/*
		std::vector<char> wif_bytes;
		try
		{
			wif_bytes = fc::from_base58(wif_key);
		}
		catch (const fc::parse_error_exception&)
		{
			return fc::optional<fc::ecc::private_key>();
		}
		if (wif_bytes.size() < 5)
			return fc::optional<fc::ecc::private_key>();

		printf("the size is  %d\n", wif_bytes.size());

		std::vector<char> key_bytes(wif_bytes.begin() + 1, wif_bytes.end() - 5);

		fc::ecc::private_key key = fc::variant(key_bytes).as<fc::ecc::private_key>();
		fc::sha256 check = fc::sha256::hash(wif_bytes.data(), wif_bytes.size() - 5);
		fc::sha256 check2 = fc::sha256::hash(check);

		if (memcmp((char*)&check, wif_bytes.data() + wif_bytes.size() - 4, 4) == 0 ||
			memcmp((char*)&check2, wif_bytes.data() + wif_bytes.size() - 4, 4) == 0)
			return key;

		return fc::optional<fc::ecc::private_key>();*/

		auto key = graphene::utilities::wif_to_key(wif_key);
		set_key(*key);
		return key;

	}
	std::string ltc_privatekey::sign_trx( const std::string& raw_trx,int index)
	{
		return this->crosschain_privatekey_base::sign_trx(raw_trx, index);
	}
	void bch_privatekey::init()
	{
		set_id(0);
		set_pubkey_prefix(btc_pubkey);
		set_script_prefix(btc_script);
		set_privkey_prefix(btc_privkey);
	}

	std::string  bch_privatekey::get_wif_key()
	{
		FC_ASSERT(is_empty() == false, "private key is empty!");

		fc::sha256 secret = get_private_key().get_secret();
		//one byte for prefix, one byte for compressed sentinel
		const size_t size_of_data_to_hash = sizeof(secret) + 2;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)get_privkey_prefix();
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		data[size_of_data_to_hash - 1] = (char)0x01;
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));

	}
	std::string   bch_privatekey::get_address()
	{
		FC_ASSERT(is_empty() == false, "private key is empty!");

		//configure for bitcoin
		uint8_t version = get_pubkey_prefix();
		bool compress = true;

		const fc::ecc::private_key& priv_key = get_private_key();
		fc::ecc::public_key  pub_key = priv_key.get_public_key();

		graphene::chain::pts_address_bch btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
	}
	std::string bch_privatekey::get_address_by_pubkey(const std::string& pub)
	{
		auto old_address = graphene::privatekey_management::get_address_by_pubkey(pub, get_pubkey_prefix());
		
		return std::string(graphene::chain::pts_address_bch(graphene::chain::pts_address(old_address), btc_pubkey, btc_script));
	}
	std::string bch_privatekey::get_public_key()
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());

		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();

		return pub_hex;

	}
	


	fc::optional<fc::ecc::private_key>   bch_privatekey::import_private_key(const std::string& wif_key)
	{
		auto key = graphene::utilities::wif_to_key(wif_key);
		set_key(*key);
		return key;

	}
	static libbitcoin::chain::script strip_code_seperators(const libbitcoin::chain::script& script_code)
	{
		libbitcoin::machine::operation::list ops;

		for (auto op = script_code.begin(); op != script_code.end(); ++op)
			if (op->code() != libbitcoin::machine::opcode::codeseparator)
				ops.push_back(*op);

		return libbitcoin::chain::script(std::move(ops));
	}
	std::string create_endorsement_bch(const std::string& signer_wif, const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index,const uint64_t& value)
	{

		libbitcoin::wallet::ec_private libbitcoin_priv(signer_wif);
		libbitcoin::chain::script   libbitcoin_script;
		libbitcoin_script.from_data(libbitcoin::config::base16(redeemscript_hex), false);
		const auto stripped = strip_code_seperators(libbitcoin_script);
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(raw_trx));
		BITCOIN_ASSERT(vin_index < trx.inputs().size());
		const auto& input = trx.inputs()[vin_index];
		//const auto size = libbitcoin::chain::preimage_size(script_code.serialized_size(true));
		uint32_t sighash_type = libbitcoin::machine::sighash_algorithm::all | 0x40;
		libbitcoin::data_chunk data;
		//data.reserve(size);
		libbitcoin::data_sink ostream(data);
		libbitcoin::ostream_writer sink(ostream);

		// Flags derived from the signature hash byte.
		//const auto sighash = to_sighash_enum(sighash_type);
		const auto any = (sighash_type & libbitcoin::machine::sighash_algorithm::anyone_can_pay) != 0;
		const auto single = (sighash_type & libbitcoin::machine::sighash_algorithm::single) != 0;
		const auto none = (sighash_type & libbitcoin::machine::sighash_algorithm::none) != 0;
		const auto all = (sighash_type & libbitcoin::machine::sighash_algorithm::all) != 0;

		// 1. transaction version (4-byte little endian).
		sink.write_little_endian(trx.version());

		// 2. inpoints hash (32-byte hash).
		sink.write_hash(!any ? trx.inpoints_hash() : libbitcoin::null_hash);

		// 3. sequences hash (32-byte hash).
		sink.write_hash(!any && all ? trx.sequences_hash() : libbitcoin::null_hash);

		// 4. outpoint (32-byte hash + 4-byte little endian).
		input.previous_output().to_data(sink);

		// 5. script of the input (with prefix).
		stripped.to_data(sink, true);
		//uint64_t value = 500000000;
		// 6. value of the output spent by this input (8-byte little endian).
		sink.write_little_endian(value);

		// 7. sequence of the input (4-byte little endian).
		sink.write_little_endian(input.sequence());

		// 8. outputs hash (32-byte hash).
		sink.write_hash(all ? trx.outputs_hash() :
			(single && vin_index < trx.outputs().size() ?
				libbitcoin::bitcoin_hash(trx.outputs()[vin_index].to_data()) : libbitcoin::null_hash));

		// 9. transaction locktime (4-byte little endian).
		sink.write_little_endian(trx.locktime());

		// 10. sighash type of the signature (4-byte [not 1] little endian).
		sink.write_4_bytes_little_endian(sighash_type);

		ostream.flush();
		auto higest = libbitcoin::bitcoin_hash(data);
		/*
		uint32_t index = vin_index;
		
		libbitcoin::chain::input::list ins;
		const auto& inputs = trx.inputs();
		std::cout <<"sig hash type is" << sighash_type << std::endl;
		const auto any = (sighash_type & libbitcoin::machine::sighash_algorithm::anyone_can_pay) != 0;
		ins.reserve(any ? 1 : inputs.size());

		BITCOIN_ASSERT(vin_index < inputs.size());
		const auto& self = inputs[vin_index];

		if (any)
		{
			// Retain only self.
			ins.emplace_back(self.previous_output(), stripped, self.sequence());
		}
		else
		{
			// Erase all input scripts.
			for (const auto& input : inputs)
				ins.emplace_back(input.previous_output(), libbitcoin::chain::script{},
					input.sequence());

			// Replace self that is lost in the loop.
			ins[vin_index].set_script(stripped);
			////ins[input_index].set_sequence(self.sequence());
		}

		// Move new inputs and copy outputs to new transaction.
		libbitcoin::chain::transaction out(trx.version(), trx.locktime(), libbitcoin::chain::input::list{}, trx.outputs());
		std::cout << "trx version is " << trx.version() << std::endl;
		out.set_inputs(std::move(ins));
		out.inpoints_hash();
		out.sequences_hash();
		auto temp = out.to_data(true, false);
		std::cout << "trx out put" << BinToHex(std::vector<char>(temp.begin(), temp.end()), false) << std::endl;create_endorsement_hc
		
		//uint32_t newForkValue = 0xff0000 | (sighash_type ^ 0xdead);
		//auto fork_signhash_type = ((newForkValue << 8) | (sighash_type & 0xff));
		auto fork_signhash_type = sighash_type;
		libbitcoin::extend_data(serialized, libbitcoin::to_little_endian(fork_signhash_type));
		std::cout << BinToHex(std::vector<char>(serialized.begin(), serialized.end()), false) << std::endl;
		*/
		//auto higest = libbitcoin::bitcoin_hash(serialized);
		//std::cout << BinToHex(std::vector<char>(higest.begin(), higest.end()), false) << std::endl;
		libbitcoin::ec_signature signature;
		libbitcoin::endorsement endorse;
		if (!libbitcoin::sign_bch(signature, libbitcoin_priv.secret(), higest) || !libbitcoin::encode_signature(endorse, signature))
			return "";
		endorse.push_back(sighash_type);
		endorse.shrink_to_fit();
		return libbitcoin::encode_base16(endorse);
	}
	std::string bch_privatekey::mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx)
	{
		try {
			FC_ASSERT(raw_trx.contains("hex"));
			FC_ASSERT(raw_trx.contains("trx"));
			auto tx = raw_trx["trx"].get_object();
			auto size = tx["vin"].get_array().size();
			auto vins = tx["vin"].get_array();
			std::string trx = raw_trx["hex"].as_string();
			for (int index = 0; index < size; index++)
			{
				FC_ASSERT(vins[index].get_object().contains("amount"),"No amount in bch vin");
				uint64_t amount = vins[index]["amount"].as_uint64();
				auto endorse = create_endorsement_bch(get_wif_key(), redeemscript, trx, index,amount);
				trx = graphene::privatekey_management::mutisign_trx(endorse, redeemscript, trx, index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));
	}
	
	std::string bch_privatekey::sign_trx(const std::string& raw_trx, int index)
	{
		auto sep_pos = raw_trx.find("|");
		FC_ASSERT(sep_pos != raw_trx.npos, "BCH need Amount");
		auto real_raw_trx = raw_trx.substr(0, sep_pos);
		auto amount = fc::variant(raw_trx.substr(sep_pos+1)).as_uint64();

		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		libbitcoin::chain::script script = libbitcoin::chain::script::to_pay_key_hash_pattern(libbitcoin_priv.to_payment_address().hash());
		auto prev = libbitcoin::encode_base16(script.to_data(false));
		auto out = create_endorsement_bch(get_wif_key(), prev, real_raw_trx, index, amount);
		FC_ASSERT(out != "");

		libbitcoin::chain::script   libbitcoin_script;//(libbitcoin::data_chunk(base),true);
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(real_raw_trx));
		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();
		std::string endorsment_script = "[" + out + "]" + " [" + pub_hex + "] ";
		libbitcoin_script.from_string(endorsment_script);
		trx.inputs()[index].set_script(libbitcoin_script);
		std::string signed_trx = libbitcoin::encode_base16(trx.to_data());
		return signed_trx;
	}
	libbitcoin::hash_digest create_digest_bch(const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index, const uint64_t& value) {
		libbitcoin::chain::transaction  trx;
		libbitcoin::chain::script   libbitcoin_script;
		libbitcoin_script.from_data(libbitcoin::config::base16(redeemscript_hex), false);
		const auto stripped = strip_code_seperators(libbitcoin_script);
		trx.from_data(libbitcoin::config::base16(raw_trx));
		BITCOIN_ASSERT(vin_index < trx.inputs().size());
		const auto& input = trx.inputs()[vin_index];
		//const auto size = libbitcoin::chain::preimage_size(script_code.serialized_size(true));
		uint32_t sighash_type = libbitcoin::machine::sighash_algorithm::all | 0x40;
		libbitcoin::data_chunk data;
		//data.reserve(size);
		libbitcoin::data_sink ostream(data);
		libbitcoin::ostream_writer sink(ostream);

		// Flags derived from the signature hash byte.
		//const auto sighash = to_sighash_enum(sighash_type);
		const auto any = (sighash_type & libbitcoin::machine::sighash_algorithm::anyone_can_pay) != 0;
		const auto single = (sighash_type & libbitcoin::machine::sighash_algorithm::single) != 0;
		const auto none = (sighash_type & libbitcoin::machine::sighash_algorithm::none) != 0;
		const auto all = (sighash_type & libbitcoin::machine::sighash_algorithm::all) != 0;

		// 1. transaction version (4-byte little endian).
		sink.write_little_endian(trx.version());

		// 2. inpoints hash (32-byte hash).
		sink.write_hash(!any ? trx.inpoints_hash() : libbitcoin::null_hash);

		// 3. sequences hash (32-byte hash).
		sink.write_hash(!any && all ? trx.sequences_hash() : libbitcoin::null_hash);

		// 4. outpoint (32-byte hash + 4-byte little endian).
		input.previous_output().to_data(sink);

		// 5. script of the input (with prefix).
		stripped.to_data(sink, true);
		//uint64_t value = 500000000;
		// 6. value of the output spent by this input (8-byte little endian).
		sink.write_little_endian(value);

		// 7. sequence of the input (4-byte little endian).
		sink.write_little_endian(input.sequence());

		// 8. outputs hash (32-byte hash).
		sink.write_hash(all ? trx.outputs_hash() :
			(single && vin_index < trx.outputs().size() ?
				libbitcoin::bitcoin_hash(trx.outputs()[vin_index].to_data()) : libbitcoin::null_hash));

		// 9. transaction locktime (4-byte little endian).
		sink.write_little_endian(trx.locktime());

		// 10. sighash type of the signature (4-byte [not 1] little endian).
		sink.write_4_bytes_little_endian(sighash_type);

		ostream.flush();
		return libbitcoin::bitcoin_hash(data);
	}
	bool bch_privatekey::validate_transaction(const std::string& addr, const std::string& redeemscript, const std::string& sig) {
		auto sep_pos = sig.find("|");
		FC_ASSERT(sep_pos != sig.npos, "BCH need Amount");
		auto real_raw_trx = sig.substr(0, sep_pos);
		auto source_raw_trx = sig.substr(sep_pos + 1);
		auto handled_source_raw_trx = fc::json::from_string(source_raw_trx).get_object();
		FC_ASSERT(handled_source_raw_trx.contains("hex"));
		FC_ASSERT(handled_source_raw_trx.contains("trx"));
		auto source_tx = handled_source_raw_trx["trx"].get_object();
		auto vins = source_tx["vin"].get_array();
		libbitcoin::chain::transaction  tx;
		tx.from_data(libbitcoin::config::base16(real_raw_trx));
		libbitcoin::wallet::ec_public libbitcoin_pub(addr);
		FC_ASSERT(libbitcoin_pub != libbitcoin::wallet::ec_public(), "the pubkey hex str is in valid!");
		libbitcoin::data_chunk pubkey_out;
		FC_ASSERT(libbitcoin_pub.to_data(pubkey_out));
		auto ins = tx.inputs();
		auto int_size = ins.size();
		FC_ASSERT(int_size == vins.size());
		uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;
		int vin_index = int_size - 1;

		for (; vin_index >= 0; vin_index--)
		{
			auto input = tx.inputs().at(vin_index);
			std::string script_str = input.script().to_string(libbitcoin::machine::all_rules);
			auto pos_first = script_str.find('[');
			FC_ASSERT(pos_first != std::string::npos);
			auto pos_end = script_str.find(']');
			FC_ASSERT(pos_end != std::string::npos);
			std::string hex = script_str.assign(script_str, pos_first + 1, pos_end - pos_first - 1);
			libbitcoin::endorsement out;
			FC_ASSERT(libbitcoin::decode_base16(out, hex));
			libbitcoin::der_signature der_sig;
			FC_ASSERT(libbitcoin::parse_endorsement(hash_type, der_sig, std::move(out)));
			libbitcoin::ec_signature ec_sig;
			FC_ASSERT(libbitcoin::parse_signature(ec_sig, der_sig, false));
			auto vin = vins[vin_index].get_object();
			FC_ASSERT(vin.contains("amount"));
			auto sigest = create_digest_bch(redeemscript, real_raw_trx, vin_index, vin["amount"].as_uint64());
			if (false == libbitcoin::verify_signature(pubkey_out, sigest, ec_sig))
				return false;
		}
		return true;
	}
	bool bch_privatekey::validate_address(const std::string& addr) {
		graphene::chain::pts_address_bch bch_addr(addr);
		return bch_addr.is_valid();
	}
	void input_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write) {
		auto InputCount = source.read_size_little_endian();
		write.write_size_little_endian(uint64_t(InputCount));
		//std::cout << InputCount << std::endl;
		for (uint64_t i = 0; i < InputCount; ++i) {
			auto Hash = source.read_hash();
			auto Index = source.read_4_bytes_little_endian();
			auto Tree = source.read_size_little_endian();
			write.write_hash(Hash);
			write.write_4_bytes_little_endian(Index);
			write.write_size_little_endian(Tree);
			auto Sequence = source.read_4_bytes_little_endian();
			write.write_4_bytes_little_endian(Sequence);
		}
	}
	void output_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write) {
		auto OutputCount = source.read_size_little_endian();
		write.write_size_little_endian((uint64_t)OutputCount);
		//std::cout << (uint64_t)OutputCount << std::endl;
		for (uint64_t i = 0; i < OutputCount; ++i) {
			auto Value = source.read_8_bytes_little_endian();
			auto Version = source.read_2_bytes_little_endian();
			auto output_count = source.read_size_little_endian();
			auto PkScript = source.read_bytes(output_count);
			write.write_8_bytes_little_endian(Value);
			write.write_2_bytes_little_endian(Version);
			write.write_size_little_endian(output_count);
			write.write_bytes(PkScript);
			//std::cout << PkScript.size() << "-" << output_count<<std::endl;
		}
	}
	void witness_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write, libbitcoin::chain::script libbitcoin_script,int vin_index,bool bHashMode = false) {
		auto witness_count = source.read_size_little_endian();
		write.write_size_little_endian((uint64_t)witness_count);
		for (uint64_t i = 0; i < (int)witness_count; ++i) {
			auto ValueIn = source.read_8_bytes_little_endian();
			auto BlockHeight = source.read_4_bytes_little_endian();
			auto BlockIndex = source.read_4_bytes_little_endian();
			auto signtureCount = source.read_size_little_endian();
			//std::cout << signtureCount << std::endl;
			auto SignatureScript = source.read_bytes(signtureCount);
			if (!bHashMode){
				write.write_8_bytes_little_endian(ValueIn);
				write.write_4_bytes_little_endian(BlockHeight);
				write.write_4_bytes_little_endian(BlockIndex);
			}
			//std::cout << SignatureScript.size() << "-" << signtureCount << std::endl;
			if (i == vin_index) {
				write.write_size_little_endian(libbitcoin_script.to_data(false).size());
				write.write_bytes(libbitcoin_script.to_data(false));
			}
			else {
				if (bHashMode){
					write.write_size_little_endian((uint8_t)0);
				}
				else {
					write.write_size_little_endian(signtureCount);
					write.write_bytes(SignatureScript);
				}
				
			}

		}
	}

	bool  from_hex(const char *pSrc, std::vector<char> &pDst, unsigned int nSrcLength, unsigned int &nDstLength)
	{
		if (pSrc == 0)
		{
			return false;
		}

		nDstLength = 0;

		if (pSrc[0] == 0) // nothing to convert  
			return 0;

		// 计算需要转换的字节数  
		for (int j = 0; pSrc[j]; j++)
		{
			if (isxdigit(pSrc[j]))
				nDstLength++;
		}

		// 判断待转换字节数是否为奇数，然后加一  
		if (nDstLength & 0x01) nDstLength++;
		nDstLength /= 2;

		if (nDstLength > nSrcLength)
			return false;

		nDstLength = 0;

		int phase = 0;
		char temp_char;

		for (int i = 0; pSrc[i]; i++)
		{
			if (!isxdigit(pSrc[i]))
				continue;

			unsigned char val = pSrc[i] - (isdigit(pSrc[i]) ? 0x30 : (isupper(pSrc[i]) ? 0x37 : 0x57));

			if (phase == 0)
			{
				temp_char = val << 4;
				phase++;
			}
			else
			{
				temp_char |= val;
				phase = 0;
				pDst.push_back(temp_char);
				nDstLength++;
			}
		}

		return true;
	}
	
	std::string bch_privatekey::sign_message(const std::string& msg)
	{
		libbitcoin::wallet::message_signature sign;

		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		// 		libbitcoin::wallet::ec_private libbitcoin_priv("L13gvvM3TtL2EmfBdye8tp4tQhcbCG3xz3VPrBjSZL8MeJavLL8K");
		libbitcoin::data_chunk  data(msg.begin(), msg.end());
		libbitcoin::data_slice message_sli(data);
		libbitcoin::recoverable_signature recoverable;
		libbitcoin::wallet::ec_private secret(libbitcoin_priv.secret());
		if (!libbitcoin::sign_recoverable(recoverable, secret, libbitcoin::wallet::hash_message(message_sli), true)) {
			return "";
		}
		uint8_t magic;
		std::cout << secret.compressed() << std::endl;
		if (!libbitcoin::wallet::recovery_id_to_magic(magic, recoverable.recovery_id, secret.compressed())) {
			return "";
		}
		sign = libbitcoin::splice(libbitcoin::to_array(magic), recoverable.signature);
		auto result = libbitcoin::encode_base64(sign);
		return result;
	}
	std::string eth_privatekey::get_address() {
		FC_ASSERT(is_empty() == false, "private key is empty!");
		auto pubkey = get_private_key().get_public_key();
		auto dat = pubkey.serialize_ecc_point();
		Keccak tmp_addr;
		tmp_addr.add(dat.begin() + 1, dat.size() - 1);
		auto addr_str_keccaksha3 = tmp_addr.getHash();
		auto hex_str = addr_str_keccaksha3.substr(24, addr_str_keccaksha3.size());
		return  "0x" + fc::ripemd160(hex_str).str();
	}
	std::string eth_privatekey::get_public_key() {
		auto pubkey = get_private_key().get_public_key();
		auto dat = pubkey.serialize_ecc_point();
		return "0x" + fc::to_hex(dat.data,dat.size());
	}
	std::string eth_privatekey::get_address_by_pubkey(const std::string& pub) {
		FC_ASSERT(pub.size() > 2, "eth pubkey size error");
		const int size_of_data_to_hash = ((pub.size() - 2) / 2);
		//FC_ASSERT(((size_of_data_to_hash == 33) || (size_of_data_to_hash == 65)), "eth pubkey size error");
		FC_ASSERT(((pub.at(0) == '0') && (pub.at(1) == 'x')), "eth pubkey start error");
		std::string pubwithout(pub.begin() + 2, pub.end());
		char pubchar[1024];
		fc::from_hex(pubwithout, pubchar, size_of_data_to_hash);
		Keccak tmp_addr;
		tmp_addr.add(pubchar + 1, size_of_data_to_hash - 1);
		auto addr_str_keccaksha3 = tmp_addr.getHash();
		auto hex_str = addr_str_keccaksha3.substr(24, addr_str_keccaksha3.size());
		return  "0x" + fc::ripemd160(hex_str).str();
	}
	std::string  eth_privatekey::sign_message(const std::string& msg) {
		auto msgHash = msg.substr(2);
		dev::Secret sec(dev::jsToBytes(get_private_key().get_secret()));
		std::string prefix = "\x19";
		prefix += "Ethereum Signed Message:\n";
		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(msgHash.data(), temp, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		std::string msg_prefix = prefix +fc::to_string(temp.size())+std::string(temp.begin(),temp.end());//std::string(temp.begin(), temp.end());
		Keccak real_hash;
		real_hash.add(msg_prefix.data(), msg_prefix.size());
		dev::h256 hash;
		msgHash = real_hash.getHash();
		std::vector<char> temp1;
		nDeplength = 0;
		b_converse = from_hex(msgHash.data(), temp1, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		for (int i = 0; i < temp1.size(); ++i)
		{
			hash[i] = temp1[i];
		}
		auto sign_str = dev::sign(sec, hash);
		std::vector<char> sign_bin(sign_str.begin(), sign_str.end());
		std::string signs = BinToHex(sign_bin, false);
		if (signs.substr(signs.size() - 2) == "00")
		{
			signs[signs.size() - 2] = '1';
			signs[signs.size() - 1] = 'b';
		}
		else if (signs.substr(signs.size() - 2) == "01")
		{
			signs[signs.size() - 2] = '1';
			signs[signs.size() - 1] = 'c';
		}
		
		//formal eth sign v
		/*if (signs.substr(signs.size() - 2) == "00")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '5';
		}
		else if (signs.substr(signs.size() - 2) == "01")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '6';
		}*/
		
		return signs;
	}

	bool  eth_privatekey::verify_message(const std::string address,const std::string& msg,const std::string& signature) {
		auto msgHash = msg.substr(2);
		std::string prefix = "\x19";
		prefix += "Ethereum Signed Message:\n";
		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(msgHash.data(), temp, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		std::string msg_prefix = prefix + fc::to_string(temp.size()) + std::string(temp.begin(), temp.end());//std::string(temp.begin(), temp.end());
		Keccak real_hash;
		real_hash.add(msg_prefix.data(), msg_prefix.size());
		dev::h256 hash;
		msgHash = real_hash.getHash();
		std::vector<char> temp1;
		nDeplength = 0;
		b_converse = from_hex(msgHash.data(), temp1, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		for (int i = 0; i < temp1.size(); ++i)
		{
			hash[i] = temp1[i];
		}
		dev::Signature sig{ signature };
		sig[64] = sig[64] - 27;

		const auto& publickey = dev::recover(sig,hash);
		auto const& dev_addr = dev::toAddress(publickey);
		if (address == "0x"+dev_addr.hex())
			return true;
		return false;

		//formal eth sign v
		/*if (signs.substr(signs.size() - 2) == "00")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '5';
		}
		else if (signs.substr(signs.size() - 2) == "01")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '6';
		}*/
	}

	std::string  eth_privatekey::sign_trx(const std::string& raw_trx, int index) {
		auto eth_trx = raw_trx;

		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(eth_trx.data(), temp, eth_trx.size(), nDeplength);
		FC_ASSERT(b_converse);
		dev::bytes trx(temp.begin(), temp.end());
		dev::eth::TransactionBase trx_base(trx, dev::eth::CheckTransaction::None);
		dev::Secret sec(dev::jsToBytes(get_private_key().get_secret()));
		trx_base.sign(sec);
		auto signed_trx = trx_base.rlp(dev::eth::WithSignature);
		std::string signed_trx_str(signed_trx.begin(), signed_trx.end());
		std::vector<char> hex_trx(signed_trx.begin(), signed_trx.end());
		return BinToHex(hex_trx, false);
	}

	std::string eth_privatekey::get_wif_key() {
		/*char buf[1024];
		::memset(buf, 0, 1024);
		sprintf_s(buf, "%x", get_private_key().get_secret().data());
		std::string eth_pre_key(buf);*/
		return  get_private_key().get_secret().str();
	}
	fc::optional<fc::ecc::private_key>  eth_privatekey::import_private_key(const std::string& wif_key) {
		//char buf[1024];
		//::memset(buf, 0, 1024);
		//fc::from_hex(wif_key,buf,wif_key.size());
		fc::ecc::private_key_secret ad(wif_key);
		fc::ecc::private_key key = fc::variant(ad).as<fc::ecc::private_key>();
		set_key(key);
		return key;
	}
	fc::variant_object  eth_privatekey::decoderawtransaction(const std::string& trx)
	{
		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(trx.data(), temp, trx.size(), nDeplength);
		FC_ASSERT(b_converse);
		dev::bytes bintrx(temp.begin(), temp.end());
		dev::eth::TransactionBase trx_base(bintrx, dev::eth::CheckTransaction::Everything);
		fc::mutable_variant_object ret_obj;
		ret_obj.set("from","0x"+trx_base.from().hex());
		ret_obj.set("to", "0x"+trx_base.to().hex());
		ret_obj.set("nonce", trx_base.nonce().str());
		ret_obj.set("gasPrice", trx_base.gasPrice().str());
		ret_obj.set("gas", trx_base.gas().str());
		ret_obj.set("value", trx_base.value().str());
		auto bin = trx_base.sha3();
		std::vector<char> vectorBin(bin.begin(), bin.end());
		auto trx_id = "0x" + BinToHex(vectorBin, false);
		ret_obj.set("trxid", trx_id);
		std::vector<char> temp_trx(trx_base.data().begin(), trx_base.data().end());
		std::string hexinput = BinToHex(temp_trx, false);
		ret_obj.set("input", hexinput);
		return fc::variant_object(ret_obj);
	};
	std::string eth_privatekey::mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx) {
		if (raw_trx.contains("without_sign_trx_sign"))
		{
			auto eth_trx = raw_trx["without_sign_trx_sign"].as_string();
		
			std::vector<char> temp;
			unsigned int nDeplength = 0;
			bool b_converse = from_hex(eth_trx.data(), temp, eth_trx.size(), nDeplength);
			FC_ASSERT(b_converse);
			dev::bytes trx(temp.begin(), temp.end());
			dev::eth::TransactionBase trx_base(trx,dev::eth::CheckTransaction::None);

			dev::Secret sec(dev::jsToBytes(get_private_key().get_secret()));
			trx_base.sign(sec);
			auto signed_trx = trx_base.rlp(dev::eth::WithSignature);
			std::string signed_trx_str(signed_trx.begin(), signed_trx.end());
			std::vector<char> hex_trx(signed_trx.begin(), signed_trx.end());
			return BinToHex(hex_trx,false);
		}
		else if (raw_trx.contains("get_param_hash"))
		{
			//std::string cointype = raw_trx["get_param_hash"]["cointype"].as_string();
			std::string cointype = raw_trx["get_param_hash"]["cointype"].as_string();
			std::string msg_address = raw_trx["get_param_hash"]["msg_address"].as_string();
			std::string msg_amount = raw_trx["get_param_hash"]["msg_amount"].as_string();
			std::string msg_prefix = raw_trx["get_param_hash"]["msg_prefix"].as_string();
			std::string msg_to_hash = msg_address + msg_amount + cointype.substr(24);
			std::vector<char> temp;
			unsigned int nDeplength = 0;
			bool b_converse = from_hex(msg_to_hash.data(), temp, msg_to_hash.size(), nDeplength);
			FC_ASSERT(b_converse);
			Keccak messageHash;
			messageHash.add(msg_prefix.data(), msg_prefix.size());
			messageHash.add(temp.data(), temp.size());
			auto msgHash = messageHash.getHash();
			return sign_message("0x"+msgHash);
		}
		else {
			std::string msg_address = raw_trx["msg_address"].as_string();
			std::string msg_amount = raw_trx["msg_amount"].as_string();
			std::string msg_prefix = raw_trx["msg_prefix"].as_string();
			std::string sign_msg = msg_prefix + msg_address + msg_amount;
			return sign_message(sign_msg);
		}
		
	}
	bool eth_privatekey::validate_address(const std::string& addr) {
		FC_ASSERT(addr.size() == 42 && addr[0] == '0' && addr[1] == 'x');
		auto addr_str = addr.substr(2, addr.size());
		for (auto i = addr_str.begin(); i != addr_str.end(); ++i)
		{
			fc::from_hex(*i);
		}
		return true;
	}
	crosschain_management::crosschain_management()
	{
		crosschain_decode.insert(std::make_pair("BTC", &graphene::privatekey_management::btc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("BCH", &graphene::privatekey_management::bch_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("LTC", &graphene::privatekey_management::ltc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("ETH", &graphene::privatekey_management::eth_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("ERC", &graphene::privatekey_management::eth_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("USDT", &graphene::privatekey_management::btc_privatekey::decoderawtransaction));
	}
	crosschain_privatekey_base * crosschain_management::get_crosschain_prk(const std::string& name)
	{
		auto itr = crosschain_prks.find(name);
		if (itr != crosschain_prks.end())
		{
			return itr->second;
		}

		if (name == "BTC")
		{
			auto itr = crosschain_prks.insert(std::make_pair(name, new btc_privatekey()));
			return itr.first->second;
		}
		else if (name == "BCH")
		{
			auto itr = crosschain_prks.insert(std::make_pair(name, new bch_privatekey()));
			return itr.first->second;
		}
		else if (name == "USDT")
		{
			auto itr = crosschain_prks.insert(std::make_pair(name, new btc_privatekey()));
			return itr.first->second;
		}
		else if (name == "LTC")
		{
			auto itr = crosschain_prks.insert(std::make_pair(name, new ltc_privatekey()));
			return itr.first->second;
		}
		else if (name == "ETH") {
			auto itr = crosschain_prks.insert(std::make_pair(name, new eth_privatekey()));
			return itr.first->second;
		}
		else if (name.find("ERC") != name.npos){
			auto itr = crosschain_prks.insert(std::make_pair(name, new eth_privatekey()));
			return itr.first->second;
		}
		return nullptr;
	}

	fc::variant_object crosschain_management::decoderawtransaction(const std::string& raw_trx, const std::string& symbol)
	{
		try {
			std::string temp_symbol = symbol;
			if (symbol.find("ERC") != symbol.npos)
			{
				temp_symbol = "ERC";
			}
			auto iter = crosschain_decode.find(temp_symbol);
			FC_ASSERT(iter != crosschain_decode.end(), "plugin not exist.");
			return iter->second(raw_trx);
		}FC_CAPTURE_AND_RETHROW((raw_trx)(symbol))
	}


} } // end namespace graphene::privatekey_management
