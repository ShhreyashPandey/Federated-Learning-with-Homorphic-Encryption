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
        // Load CryptoContext
        CryptoContext<DCRTPoly> cc;
        std::ifstream ccFile("cc.bin", std::ios::binary);
        if (!ccFile) {
            std::cerr << "[c2_decrypt] ERROR: could not open cc.bin\n";
            return 1;
        }
        Serial::Deserialize(cc, ccFile, SerType::BINARY);
        ccFile.close();

        // Load private key
        PrivateKey<DCRTPoly> privKey;
        std::ifstream skFile("client2_data/client2_private.key", std::ios::binary);
        if (!skFile) {
            std::cerr << "[c2_decrypt] ERROR: could not open private key\n";
            return 1;
        }
        Serial::Deserialize(privKey, skFile, SerType::BINARY);
        skFile.close();

        // Load round number
        std::ifstream roundFile("round_counter.txt");
        if (!roundFile) {
            std::cerr << "[c2_decrypt] ERROR: could not read round_counter.txt\n";
            return 1;
        }
        int roundnum = 0;
        roundFile >> roundnum;
        roundFile.close();

        // Get aggregated encrypted params from server
        std::string url = "http://localhost:8000/s2c/agg_params?client_id=client2&round=" + std::to_string(roundnum);
        std::string response = HttpGetJson(url);
        json resp_json = json::parse(response);

        if (!resp_json.contains("agg_params")) {
            std::cerr << "[c2_decrypt] ERROR: agg_params missing in server response\n";
            return 1;
        }
        std::string b64_params = resp_json["agg_params"];
        std::vector<uint8_t> param_bytes = Base64Decode(b64_params);

        std::stringstream ss;
        ss.write(reinterpret_cast<const char*>(param_bytes.data()), param_bytes.size());

        std::vector<Ciphertext<DCRTPoly>> ciphertexts;
        Serial::Deserialize(ciphertexts, ss, SerType::BINARY);

        // Decrypt w and c
        Plaintext ptW, ptC;
        cc->Decrypt(privKey, ciphertexts[0], &ptW);
        cc->Decrypt(privKey, ciphertexts[1], &ptC);

        ptW->SetLength(1);
        ptC->SetLength(1);

        double w = ptW->GetCKKSPackedValue()[0].real();
        double c = ptC->GetCKKSPackedValue()[0].real();

        std::cout << "[c2_decrypt] Decrypted aggregated params: w=" << w << ", c=" << c << std::endl;

        // Save decrypted parameters to JSON for Python warm start
        json out_json = {{"w", w}, {"c", c}};

        std::ofstream outFile("client2_data/agg_wc.json");
        if (!outFile) {
            std::cerr << "[c2_decrypt] ERROR: could not write agg_wc.json\n";
            return 1;
        }
        outFile << out_json.dump(4);
        outFile.close();

        std::cout << "[c2_decrypt] Saved decrypted aggregated params to agg_wc.json\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[c2_decrypt] Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

