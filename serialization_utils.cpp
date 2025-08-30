#include "serialization_utils.h"
#include "base64_utils.h"

#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"
#include "pke/key/key-ser.h"
#include "pke/ciphertext-ser.h"

#include <sstream>
#include <vector>
#include <iostream>

using namespace lbcrypto;

// Serialize a single ciphertext to Base64 string
std::string SerializeCiphertextToBase64(const Ciphertext<DCRTPoly>& ct) {
    std::ostringstream oss;
    Serial::Serialize(ct, oss, SerType::BINARY);
    const std::string& bin = oss.str();

    std::cout << "[Debug] Raw ciphertext byte size = " << bin.size() << std::endl;
    if (bin.size() > 5 * 1024 * 1024) {
        throw std::runtime_error("❌ Ciphertext too large — likely corruption or serialization error");
    }

    std::vector<uint8_t> byteData;
    byteData.reserve(bin.size());
    for (char ch : bin) byteData.push_back(static_cast<uint8_t>(ch));
    std::string b64 = Base64Encode(byteData);

    std::cout << "[Debug] Base64 size: " << b64.size() << " bytes" << std::endl;
    return b64;
}

// Deserialize a single ciphertext from Base64 string
Ciphertext<DCRTPoly> DeserializeCiphertextFromBase64(const std::string& base64) {
    std::vector<uint8_t> decoded = Base64Decode(base64);
    std::string decodedStr(decoded.begin(), decoded.end());
    std::stringstream ss(decodedStr);
    Ciphertext<DCRTPoly> ct;
    Serial::Deserialize(ct, ss, SerType::BINARY);
    return ct;
}

// Serialize PublicKey to Base64 string
std::string SerializePublicKeyToBase64(const PublicKey<DCRTPoly>& pk) {
    std::stringstream ss;
    Serial::Serialize(pk, ss, SerType::BINARY);
    std::string bin = ss.str();

    std::vector<uint8_t> byteData;
    byteData.reserve(bin.size());
    for (char ch : bin) byteData.push_back(static_cast<uint8_t>(ch));
    return Base64Encode(byteData);
}

// Deserialize PublicKey from Base64 string
PublicKey<DCRTPoly> DeserializePublicKeyFromBase64(const std::string& base64) {
    std::vector<uint8_t> decoded = Base64Decode(base64);
    std::string decodedStr(decoded.begin(), decoded.end());
    std::stringstream ss(decodedStr);
    PublicKey<DCRTPoly> pk;
    Serial::Deserialize(pk, ss, SerType::BINARY);
    return pk;
}

// Serialize PrivateKey to Base64
std::string SerializePrivateKeyToBase64(const PrivateKey<DCRTPoly>& sk) {
    std::stringstream ss;
    Serial::Serialize(sk, ss, SerType::BINARY);
    std::string bin = ss.str();

    std::vector<uint8_t> byteData;
    byteData.reserve(bin.size());
    for (char ch : bin) byteData.push_back(static_cast<uint8_t>(ch));
    return Base64Encode(byteData);
}

// Deserialize PrivateKey from Base64
PrivateKey<DCRTPoly> DeserializePrivateKeyFromBase64(const std::string& base64) {
    std::vector<uint8_t> decoded = Base64Decode(base64);
    std::string decodedStr(decoded.begin(), decoded.end());
    std::stringstream ss(decodedStr);
    PrivateKey<DCRTPoly> sk;
    Serial::Deserialize(sk, ss, SerType::BINARY);
    return sk;
}

// Serialize EvalKey to Base64
std::string SerializeEvalKeyToBase64(const EvalKey<DCRTPoly>& rk) {
    std::stringstream ss;
    Serial::Serialize(rk, ss, SerType::BINARY);
    std::string bin = ss.str();

    std::vector<uint8_t> byteData;
    byteData.reserve(bin.size());
    for (char ch : bin) byteData.push_back(static_cast<uint8_t>(ch));
    return Base64Encode(byteData);
}

// Deserialize EvalKey from Base64
EvalKey<DCRTPoly> DeserializeEvalKeyFromBase64(const std::string& base64) {
    std::vector<uint8_t> decoded = Base64Decode(base64);
    std::string decodedStr(decoded.begin(), decoded.end());
    std::stringstream ss(decodedStr);
    EvalKey<DCRTPoly> rk;
    Serial::Deserialize(rk, ss, SerType::BINARY);
    return rk;
}

// Serialize EvalMultKey to Base64
std::string SerializeEvalMultKeyToBase64(const CryptoContext<DCRTPoly>& cc) {
    std::stringstream ss;
    cc->SerializeEvalMultKey(ss, SerType::BINARY, "");
    std::string bin = ss.str();

    std::vector<uint8_t> byteData;
    byteData.reserve(bin.size());
    for (char ch : bin) byteData.push_back(static_cast<uint8_t>(ch));
    return Base64Encode(byteData);
}

// Serialize EvalSumKey to Base64
std::string SerializeEvalSumKeyToBase64(const CryptoContext<DCRTPoly>& cc) {
    std::stringstream ss;
    cc->SerializeEvalSumKey(ss, SerType::BINARY);
    std::string bin = ss.str();

    std::vector<uint8_t> byteData;
    byteData.reserve(bin.size());
    for (char ch : bin) byteData.push_back(static_cast<uint8_t>(ch));
    return Base64Encode(byteData);
}

// Serialize a vector of Ciphertext to Base64 string (e.g., multi-array weights)
std::string SerializeCiphertextVectorToBase64(const std::vector<Ciphertext<DCRTPoly>>& cts) {
    std::stringstream ss;
    Serial::Serialize(cts, ss, SerType::BINARY);
    std::string bin = ss.str();

    std::vector<uint8_t> byteData;
    byteData.reserve(bin.size());
    for (char ch : bin) byteData.push_back(static_cast<uint8_t>(ch));
    return Base64Encode(byteData);
}

// Deserialize Base64 string to vector of Ciphertext (multi-array)
std::vector<Ciphertext<DCRTPoly>> DeserializeCiphertextVectorFromBase64(const std::string& base64) {
    std::vector<uint8_t> decoded = Base64Decode(base64);
    std::string decodedStr(decoded.begin(), decoded.end());
    std::stringstream ss(decodedStr);
    std::vector<Ciphertext<DCRTPoly>> cts;
    Serial::Deserialize(cts, ss, SerType::BINARY);
    return cts;
}

