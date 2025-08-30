#pragma once

#include "openfhe.h"
#include <string>
#include <vector>

// Serialization and Base64 encoding for single ciphertext
std::string SerializeCiphertextToBase64(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& ct);
lbcrypto::Ciphertext<lbcrypto::DCRTPoly> DeserializeCiphertextFromBase64(const std::string& base64);

// Serialization and Base64 encoding for PublicKey
std::string SerializePublicKeyToBase64(const lbcrypto::PublicKey<lbcrypto::DCRTPoly>& pk);
lbcrypto::PublicKey<lbcrypto::DCRTPoly> DeserializePublicKeyFromBase64(const std::string& base64);

// Serialization and Base64 encoding for PrivateKey
std::string SerializePrivateKeyToBase64(const lbcrypto::PrivateKey<lbcrypto::DCRTPoly>& sk);
lbcrypto::PrivateKey<lbcrypto::DCRTPoly> DeserializePrivateKeyFromBase64(const std::string& base64);

// Serialization and Base64 encoding for EvalKey
std::string SerializeEvalKeyToBase64(const lbcrypto::EvalKey<lbcrypto::DCRTPoly>& rk);
lbcrypto::EvalKey<lbcrypto::DCRTPoly> DeserializeEvalKeyFromBase64(const std::string& base64);

// Serialization and Base64 encoding for EvalMult/EvalSumKeys
std::string SerializeEvalMultKeyToBase64(const lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc);
std::string SerializeEvalSumKeyToBase64(const lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc);

// Serialize and deserialize vector of ciphertexts (multi-array of model weights)
std::string SerializeCiphertextVectorToBase64(const std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& cts);
std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> DeserializeCiphertextVectorFromBase64(const std::string& base64);

