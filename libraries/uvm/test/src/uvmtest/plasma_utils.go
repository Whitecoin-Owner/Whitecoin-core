package main

import (
	"bytes"
	"crypto/sha256"

	"github.com/2tvenom/cbor"
)

// CreateChildChainTx : create plasma child chain's tx
func CreateChildChainTx(ownerAddr string, ownerPubKey string, slotHex string, balance int, prevBlock int) map[string]interface{} {
	tx := make(map[string]interface{})
	tx["ownerPubKey"] = ownerPubKey
	tx["owner"] = ownerAddr
	tx["slot"] = slotHex
	tx["balance"] = balance
	tx["prevBlock"] = prevBlock
	return tx
}

// ComputeChildChainDepositTxHash : create plasma child chain's deposit tx hash(=sha256(slot))
func ComputeChildChainDepositTxHash(slotHex string) ([]byte, error) {
	var encodeBuffer bytes.Buffer
	encoder := cbor.NewEncoder(&encodeBuffer)
	if ok, encodeErr := encoder.Marshal(string(slotHex)); !ok {
		return []byte{}, encodeErr
	}
	txHashBytes := sha256.Sum256(encodeBuffer.Bytes())
	txHash := txHashBytes[:]
	return txHash, nil
}

func ComputeChildChainCommonTxHash(txBytes []byte) ([]byte, error) {
	txHashBytes := sha256.Sum256(txBytes)
	txHash := txHashBytes[:]
	return txHash, nil
}

// EncodeChildChainTx : encode plasma child chain's tx by cbor encoder
func EncodeChildChainTx(tx map[string]interface{}) ([]byte, error) {
	var encodeBuffer bytes.Buffer
	encoder := cbor.NewEncoder(&encodeBuffer)
	if ok, encodeErr := encoder.Marshal(tx); !ok {
		return []byte{}, encodeErr
	}
	txBytes := encodeBuffer.Bytes()
	return txBytes, nil
}
