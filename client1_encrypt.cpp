#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "pke/key/key-ser.h"
#include "base64_utils.h"
#include "curl_utils.h"
#include "serialization_utils.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <functional>
#include <algorithm>

using namespace lbcrypto;
using json = nlohmann::json;

int main() {
    try {
        // Load full LSTM weights JSON (list of arrays) for client1
        std::ifstream infile("client1_data/wc.json");
        if (!infile) {
            std::cerr << "[c1_encrypt] ERROR: Could not open wc.json\n";
            return 1;
        }
        json weights_json;
        infile >> weights_json;
        infile.close();

        // Load CryptoContext
        CryptoContext<DCRTPoly> cc;
        std::ifstream ccFile("cc.bin", std::ios::binary);
        if (!ccFile) {
            std::cerr << "[c1_encrypt] ERROR: Could not open cc.bin\n";
            return 1;
        }
        Serial::Deserialize(cc, ccFile, SerType::BINARY);
        ccFile.close();

        // Load public key for client1
        PublicKey<DCRTPoly> pubKey;
        std::ifstream pkFile("client1_data/client1_public.key", std::ios::binary);
        if (!pkFile) {
            std::cerr << "[c1_encrypt] ERROR: Could not open client1 public key\n";
            return 1;
        }
        Serial::Deserialize(pubKey, pkFile, SerType::BINARY);
        pkFile.close();

        // Maximum number of slots per ciphertext for CKKS = ringDim/2
        size_t ringDim = cc->GetRingDimension();
        std::cout << "[c1_encrypt] Ring dimension: " << ringDim << std::endl;
        size_t max_chunk_size = ringDim / 2;

        // Lambda: recursively flatten nested JSON arrays of doubles into flat vector
        std::function<void(const json&, std::vector<double>&)> flatten_json = [&](const json& j, std::vector<double>& out_vec) {
            if (j.is_array()) {
                for (const auto& el : j) {
                    flatten_json(el, out_vec);
                }
            } else if (j.is_number_float() || j.is_number_integer()) {
                out_vec.push_back(j.get<double>());
            } else {
                throw std::runtime_error("[c1_encrypt] Non-numeric element inside weights JSON.");
            }
        };

        std::vector<Ciphertext<DCRTPoly>> ciphertexts;
        std::vector<size_t> chunkCounts;   // Number of chunks per weight array
        std::vector<size_t> origSizes;     // The original flattened length per array

        for (const auto& weight_array : weights_json) {
            std::vector<double> flat_weights;
            flatten_json(weight_array, flat_weights);

            origSizes.push_back(flat_weights.size()); // Save original size

            size_t chunks_for_array = 0;
            // Chunk flat_weights vector into ≤ max_chunk_size
            for (size_t start = 0; start < flat_weights.size(); start += max_chunk_size) {
                size_t chunk_len = std::min(max_chunk_size, flat_weights.size() - start);
                std::vector<double> chunk(flat_weights.begin() + start, flat_weights.begin() + start + chunk_len);

                Plaintext pt = cc->MakeCKKSPackedPlaintext(chunk);
                auto ct = cc->Encrypt(pubKey, pt);
                ciphertexts.push_back(ct);

                chunks_for_array++;
            }

            chunkCounts.push_back(chunks_for_array);
        }

        // Serialize & Base64 encode ciphertext vector
        std::string b64_params = SerializeCiphertextVectorToBase64(ciphertexts);

        // Read current round number
        std::ifstream roundFile("round_counter.txt");
        if (!roundFile) {
            std::cerr << "[c1_encrypt] ERROR: Could not read round_counter.txt\n";
            return 1;
        }
        int roundnum = 0;
        roundFile >> roundnum;
        roundFile.close();

        // Prepare JSON payload with explicit "metadata" and "data" keys, now including origSizes
        json payload = {
            {"metadata", {
                {"client_id", "client1"},
                {"round", roundnum},
                {"model_name", "LSTM"}
            }},
            {"data", {
                {"params", b64_params},
                {"chunk_counts", chunkCounts},
                {"orig_sizes", origSizes}
            }}
        };

        // Log communication upload size (payload size in bytes) to CSV (no headers)
        std::string payload_str = payload.dump();
        size_t payload_size = payload_str.size();

        std::ofstream logFile("client1_data/comm_logs.csv", std::ios_base::app);
        logFile << roundnum << ",client1,upload," << payload_size << "\n";
        logFile.close();

        // POST encrypted weights to server
        std::string response = HttpPostJson("http://localhost:8000/c2s/params", payload_str);
        std::cout << "[c1_encrypt] POST response: " << response << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[c1_encrypt] Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

