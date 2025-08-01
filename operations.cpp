#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "pke/key/key-ser.h"

#include "base64_utils.h"
#include "serialization_utils.h"
#include "curl_utils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace lbcrypto;

int main() {
    try {
        // Load CryptoContext
        CryptoContext<DCRTPoly> cc;
        std::ifstream ccIn("cc.bin", std::ios::binary);
        if (!ccIn.is_open()) {
            std::cerr << "[operations] ERROR: could not open cc.bin\n";
            return 1;
        }
        Serial::Deserialize(cc, ccIn, SerType::BINARY);
        ccIn.close();

        // Read current round number
        std::ifstream roundFile("round_counter.txt");
        if (!roundFile.is_open()) {
            std::cerr << "[operations] ERROR: could not open round_counter.txt\n";
            return 1;
        }
        int round = 0;
        roundFile >> round;
        roundFile.close();
        std::cout << "[operations] Current round: " << round << "\n";

        // Fetch encrypted parameters from both clients
        std::string params_url = "http://localhost:8000/s2c/params?round=" + std::to_string(round);
        auto all_params_json = json::parse(HttpGetJson(params_url));

        if (!all_params_json.contains("client1") || !all_params_json.contains("client2")) {
            std::cerr << "[operations] ERROR: params for both clients not found in server response\n";
            return 1;
        }

        // Deserialize ciphertext vectors for both clients
        std::vector<Ciphertext<DCRTPoly>> c1_ct_vec = DeserializeCiphertextVectorFromBase64(all_params_json["client1"]);
        std::vector<Ciphertext<DCRTPoly>> c2_ct_vec = DeserializeCiphertextVectorFromBase64(all_params_json["client2"]);

        if (c1_ct_vec.size() != 2 || c2_ct_vec.size() != 2) {
            std::cerr << "[operations] ERROR: Expected exactly two ciphertexts (w,c) per client\n";
            return 1;
        }

        // Fetch re-encryption keys from server
        std::string rk_c2_to_c1_b64 = json::parse(HttpGetJson("http://localhost:8000/s2c/rekey?from=client2&to=client1"))["rekey"];
        std::string rk_c1_to_c2_b64 = json::parse(HttpGetJson("http://localhost:8000/s2c/rekey?from=client1&to=client2"))["rekey"];

        EvalKey<DCRTPoly> rk_c2_to_c1 = DeserializeEvalKeyFromBase64(rk_c2_to_c1_b64);
        EvalKey<DCRTPoly> rk_c1_to_c2 = DeserializeEvalKeyFromBase64(rk_c1_to_c2_b64);

        // Re-encrypt client2 ciphertexts into client1's key domain
        for (size_t i = 0; i < 2; ++i) {
            c2_ct_vec[i] = cc->ReEncrypt(c2_ct_vec[i], rk_c2_to_c1);
        }

        // Perform homomorphic averaging in client1's key domain: avg = 0.5 * (c1 + c2)
        std::vector<Ciphertext<DCRTPoly>> avg_ct_vec(2);
        auto pt_half = cc->MakeCKKSPackedPlaintext(std::vector<double>{0.5});

        for (size_t i = 0; i < 2; ++i) {
            auto sum_ct = cc->EvalAdd(c1_ct_vec[i], c2_ct_vec[i]);
            avg_ct_vec[i] = cc->EvalMult(sum_ct, pt_half);
        }

        // Re-encrypt aggregated ciphertext from client1 domain to client2 domain
        std::vector<Ciphertext<DCRTPoly>> avg_ct_vec_c2(2);
        for (size_t i = 0; i < 2; ++i) {
            avg_ct_vec_c2[i] = cc->ReEncrypt(avg_ct_vec[i], rk_c1_to_c2);
        }

        // Serialize aggregated ciphertext vectors to Base64 strings
        std::string agg_params_c1_b64 = SerializeCiphertextVectorToBase64(avg_ct_vec);
        std::string agg_params_c2_b64 = SerializeCiphertextVectorToBase64(avg_ct_vec_c2);

        // Prepare aggregate payload JSON with mapped clients
        json agg_post_payload = {
            {"round", round},
            {"agg_params", {
                {"client1", agg_params_c1_b64},
                {"client2", agg_params_c2_b64}
            }}
        };


        // POST aggregated encrypted params back to server
        std::string post_url = "http://localhost:8000/c2s/server/agg_params";
        std::string post_response = HttpPostJson(post_url, agg_post_payload.dump());

        std::cout << "[operations] POST aggregated params response: " << post_response << std::endl;

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "[operations] Exception: " << e.what() << "\n";
        return 1;
    }
}

