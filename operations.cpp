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

        // Get current round
        std::ifstream roundFile("round_counter.txt");
        if (!roundFile.is_open()) {
            std::cerr << "[operations] ERROR: could not open round_counter.txt\n";
            return 1;
        }
        int round;
        roundFile >> round;
        roundFile.close();

        std::cout << "[operations] Current round: " << round << "\n";

        // Fetch encrypted params for this round from server
        std::string params_url = "http://localhost:8000/s2c/params?round=" + std::to_string(round);
        json all_params = json::parse(HttpGetJson(params_url));

        // Check client params presence
        if (!all_params.contains("client1") || !all_params.contains("client2")) {
            std::cerr << "[operations] ERROR: missing client params in response\n";
            return 1;
        }

        // Deserialize ciphertext vectors for each client
        std::vector<Ciphertext<DCRTPoly>> c1_ct_vec = DeserializeCiphertextVectorFromBase64(all_params["client1"]);
        std::vector<Ciphertext<DCRTPoly>> c2_ct_vec = DeserializeCiphertextVectorFromBase64(all_params["client2"]);

        // Ensure matching array counts
        if (c1_ct_vec.size() != c2_ct_vec.size()) {
            std::cerr << "[operations] ERROR: client ciphertext vector size mismatch: "
                      << c1_ct_vec.size() << " vs " << c2_ct_vec.size() << "\n";
            return 1;
        }

        size_t num_ct = c1_ct_vec.size();

        // Fetch proxy re-encryption keys for both directions
        std::string rk_c2_to_c1_b64 = json::parse(HttpGetJson("http://localhost:8000/s2c/rekey?from=client2&to=client1"))["rekey"];
        std::string rk_c1_to_c2_b64 = json::parse(HttpGetJson("http://localhost:8000/s2c/rekey?from=client1&to=client2"))["rekey"];

        EvalKey<DCRTPoly> rk_c2_to_c1 = DeserializeEvalKeyFromBase64(rk_c2_to_c1_b64);
        EvalKey<DCRTPoly> rk_c1_to_c2 = DeserializeEvalKeyFromBase64(rk_c1_to_c2_b64);

        // Re-encrypt client2's ciphertexts to client1's domain
        for (size_t i = 0; i < num_ct; i++) {
            c2_ct_vec[i] = cc->ReEncrypt(c2_ct_vec[i], rk_c2_to_c1);
        }

        std::vector<Ciphertext<DCRTPoly>> avg_ct_vec(num_ct);
        Plaintext half = cc->MakeCKKSPackedPlaintext(std::vector<double>{0.5});

        // Homomorphic averaging in client1 domain
        for (size_t i = 0; i < num_ct; i++) {
            auto sum = cc->EvalAdd(c1_ct_vec[i], c2_ct_vec[i]);
            avg_ct_vec[i] = cc->EvalMult(sum, half);
        }

        // Re-encrypt averaged ciphertexts to client2's domain
        std::vector<Ciphertext<DCRTPoly>> avg_ct_vec_c2(num_ct);
        for (size_t i = 0; i < num_ct; i++) {
            avg_ct_vec_c2[i] = cc->ReEncrypt(avg_ct_vec[i], rk_c1_to_c2);
        }

        // Serialize and Base64 encode aggregated ciphertext vectors
        std::string agg_c1 = SerializeCiphertextVectorToBase64(avg_ct_vec);
        std::string agg_c2 = SerializeCiphertextVectorToBase64(avg_ct_vec_c2);

        // Construct JSON payload for server
        json payload = {
            {"round", round},
            {"agg_params", {
                {"client1", agg_c1},
                {"client2", agg_c2}
            }}
        };

        // POST aggregated encrypted weights to server
        std::string post_url = "http://localhost:8000/c2s/server/agg_params";
        std::string resp = HttpPostJson(post_url, payload.dump());

        std::cout << "[operations] POST response: " << resp << std::endl;

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "[operations] Exception: " << e.what() << "\n";
        return 1;
    }
}

