#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "pke/key/key-ser.h"
#include "base64_utils.h"
#include "curl_utils.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace lbcrypto;
using json = nlohmann::json;

int main() {
    try {
        // Read w,c from wc.json saved by Python training
        std::ifstream infile("client1_data/wc.json");
        if (!infile) {
            std::cerr << "[c1_encrypt] ERROR: Could not open wc.json\n";
            return 1;
        }

        json param_json;
        infile >> param_json;
        infile.close();

        if (!param_json.contains("w") || !param_json.contains("c")) {
            std::cerr << "[c1_encrypt] ERROR: wc.json missing w or c\n";
            return 1;
        }

        double w = param_json["w"];
        double c = param_json["c"];

        // Load CryptoContext from cc.bin
        CryptoContext<DCRTPoly> cc;
        std::ifstream ccFile("cc.bin", std::ios::binary);
        if (!ccFile) {
            std::cerr << "[c1_encrypt] ERROR: Could not open cc.bin\n";
            return 1;
        }
        Serial::Deserialize(cc, ccFile, SerType::BINARY);
        ccFile.close();

        // Load public key for encryption
        PublicKey<DCRTPoly> pubKey;
        std::ifstream pkFile("client1_data/client1_public.key", std::ios::binary);
        if (!pkFile) {
            std::cerr << "[c1_encrypt] ERROR: Could not open public key\n";
            return 1;
        }
        Serial::Deserialize(pubKey, pkFile, SerType::BINARY);
        pkFile.close();

        // Encrypt parameters using public key
        Plaintext ptW = cc->MakeCKKSPackedPlaintext(std::vector<double>{w});
        Plaintext ptC = cc->MakeCKKSPackedPlaintext(std::vector<double>{c});
        auto ctW = cc->Encrypt(pubKey, ptW);
        auto ctC = cc->Encrypt(pubKey, ptC);

        std::vector<Ciphertext<DCRTPoly>> ciphertexts = {ctW, ctC};

        // Serialize ciphertexts to binary string stream
        std::stringstream ss;
        Serial::Serialize(ciphertexts, ss, SerType::BINARY);

        // Base64 encode serialized ciphertexts
        std::string serialized_str = ss.str();
        std::string b64_params = Base64Encode(std::vector<uint8_t>(serialized_str.begin(), serialized_str.end()));

        // Load round number from round_counter.txt
        std::ifstream roundFile("round_counter.txt");
        if (!roundFile) {
            std::cerr << "[c1_encrypt] ERROR: Could not read round_counter.txt\n";
            return 1;
        }
        int roundnum = 0;
        roundFile >> roundnum;
        roundFile.close();

        // Prepare JSON payload
        json payload = {
            {"client_id", "client1"},
            {"round", roundnum},
            {"params", b64_params}
        };

        // POST encrypted parameters JSON payload to server
        std::string response = HttpPostJson("http://localhost:8000/c2s/params", payload.dump());
        std::cout << "[c1_encrypt] POST response: " << response << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[c1_encrypt] Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

