#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "pke/key/key-ser.h"
#include "base64_utils.h"
#include "curl_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace lbcrypto;

int main() {
    try {
        // Load CryptoContext
        CryptoContext<DCRTPoly> cc;
        std::ifstream ccIn("cc.bin", std::ios::binary);
        if (!ccIn.is_open()) {
            std::cerr << "[client1_rekeygen] cc.bin not found.\n";
            return 1;
        }
        Serial::Deserialize(cc, ccIn, SerType::BINARY);
        ccIn.close();
        std::cout << "[client1_rekeygen] CryptoContext loaded.\n";

        // Load Client1's private key
        std::ifstream skIn("client1_data/client1_private.key", std::ios::binary);
        if (!skIn.is_open()) {
            std::cerr << "[client1_rekeygen] client1 private key not found.\n";
            return 1;
        }
        PrivateKey<DCRTPoly> client1_sk;
        Serial::Deserialize(client1_sk, skIn, SerType::BINARY);
        skIn.close();
        std::cout << "[client1_rekeygen] Private key for client1 loaded.\n";

        // GET client2's public key from server
        std::string url = "http://localhost:8000/s2c/public_key?client_id=client2";
        std::string response_str = HttpGetJson(url);

        json response = json::parse(response_str);
        if (!response.contains("public_key")) {
            std::cerr << "[client1_rekeygen] Response missing public key!\n";
            return 1;
        }

        std::string client2_pub_b64 = response["public_key"];
        std::vector<uint8_t> pub_vec = Base64Decode(client2_pub_b64);
        std::stringstream ss;
        ss.write(reinterpret_cast<const char*>(pub_vec.data()), pub_vec.size());

        PublicKey<DCRTPoly> client2_pk;
        Serial::Deserialize(client2_pk, ss, SerType::BINARY);
        std::cout << "[client1_rekeygen] Public key for client2 loaded from server.\n";

        // Generate proxy re-encryption key (from client1 to client2)
        auto rekey = cc->ReKeyGen(client1_sk, client2_pk);
        std::cout << "[client1_rekeygen] ReEncryption Key generated.\n";

        // Serialize rekey to base64
        std::stringstream rkStream;
        Serial::Serialize(rekey, rkStream, SerType::BINARY);
        std::vector<uint8_t> rk_vec((std::istreambuf_iterator<char>(rkStream)), {});
        std::string rk_b64 = Base64Encode(rk_vec);

        // POST rekey to server
        json postPayload;
        postPayload["from_client_id"] = "client1";
        postPayload["to_client_id"]   = "client2";
        postPayload["rekey"]          = rk_b64;

        std::string post_url = "http://localhost:8000/c2s/rekey";
        auto post_resp = HttpPostJson(post_url, postPayload.dump());

        std::cout << "[client1_rekeygen] ReKey posted to server.\n";
        std::cout << "[client1_rekeygen] Server Response: " << post_resp << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[client1_rekeygen] Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

