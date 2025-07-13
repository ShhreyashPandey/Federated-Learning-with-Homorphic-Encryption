#pragma once

#include "openfhe.h"
#include <string>
#include <vector>

// Base64 wrappers for ciphertexts and keys
std::string SerializeCiphertextToBase64(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& ct);
lbcrypto::Ciphertext<lbcrypto::DCRTPoly> DeserializeCiphertextFromBase64(const std::string& base64);

std::string SerializePublicKeyToBase64(const lbcrypto::PublicKey<lbcrypto::DCRTPoly>& pk);
lbcrypto::PublicKey<lbcrypto::DCRTPoly> DeserializePublicKeyFromBase64(const std::string& base64);

std::string SerializePrivateKeyToBase64(const lbcrypto::PrivateKey<lbcrypto::DCRTPoly>& sk);
lbcrypto::PrivateKey<lbcrypto::DCRTPoly> DeserializePrivateKeyFromBase64(const std::string& base64);

std::string SerializeEvalKeyToBase64(const lbcrypto::EvalKey<lbcrypto::DCRTPoly>& rk);
lbcrypto::EvalKey<lbcrypto::DCRTPoly> DeserializeEvalKeyFromBase64(const std::string& base64);
std::string SerializeEvalMultKeyToBase64(const lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc);
std::string SerializeEvalSumKeyToBase64(const lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc);
