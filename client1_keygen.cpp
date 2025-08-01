#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "pke/key/key-ser.h"
#include "serialization_utils.h"
#include "curl_utils.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace lbcrypto;
using json = nlohmann::json;

int main() {
    try {
        CryptoContext<DCRTPoly> cc;
        std::ifstream in("cc.bin", std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "Error opening cc.bin" << std::endl;
            return 1;
        }
        Serial::Deserialize(cc, in, SerType::BINARY);
        in.close();

        auto kp = cc->KeyGen();
        auto pubkey = kp.publicKey;
        auto privkey = kp.secretKey;

        cc->EvalMultKeyGen(privkey);
        cc->EvalSumKeyGen(privkey);

        // Save public key locally
        std::ofstream pkOut("client1_data/client1_public.key", std::ios::binary);
        if (!pkOut.is_open()) {
            std::cerr << "Unable to write public key" << std::endl;
            return 1;
        }
        Serial::Serialize(pubkey, pkOut, SerType::BINARY);
        pkOut.close();

        std::string pk_b64 = SerializePublicKeyToBase64(pubkey);
        std::string evm_b64 = SerializeEvalMultKeyToBase64(cc);
        std::string evs_b64 = SerializeEvalSumKeyToBase64(cc);

        // Save private key locally
        std::ofstream skOut("client1_data/client1_private.key", std::ios::binary);
        if (!skOut.is_open()) {
            std::cerr << "Unable to write private key" << std::endl;
            return 1;
        }
        Serial::Serialize(privkey, skOut, SerType::BINARY);
        skOut.close();

        json payload;
        payload["client_id"] = "client1";
        payload["public_key"] = pk_b64;
        payload["eval_mult_key"] = evm_b64;
        payload["eval_sum_key"] = evs_b64;

        auto response = HttpPostJson("http://localhost:8000/c2s/public_key", payload.dump());

        std::cout << "Public and eval keys posted, server response: " << response << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

