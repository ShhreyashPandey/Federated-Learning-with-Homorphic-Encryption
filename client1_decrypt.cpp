#include "serialization_utils.h"
#include "curl_utils.h"
#include "base64_utils.h"

#include "openfhe.h"
#include "cryptocontext.h"
#include "cryptocontext-ser.h"
#include "pke/key/key-ser.h"
#include "pke/ciphertext.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <complex>
#include <nlohmann/json.hpp>
#include <algorithm>

using namespace lbcrypto;
using json = nlohmann::json;

int main() {
    try {
        // Load CryptoContext
        CryptoContext<DCRTPoly> cc;
        std::ifstream ccFile("cc.bin", std::ios::binary);
        if (!ccFile) {
            std::cerr << "[c1_decrypt] ERROR: could not open cc.bin\n";
            return 1;
        }
        Serial::Deserialize(cc, ccFile, SerType::BINARY);
        ccFile.close();

        // Load private key of client1
        PrivateKey<DCRTPoly> privKey;
        std::ifstream skFile("client1_data/client1_private.key", std::ios::binary);
        if (!skFile) {
            std::cerr << "[c1_decrypt] ERROR: could not open client1_private.key\n";
            return 1;
        }
        Serial::Deserialize(privKey, skFile, SerType::BINARY);
        skFile.close();

        // Get current round number
        std::ifstream roundFile("round_counter.txt");
        if (!roundFile) {
            std::cerr << "[c1_decrypt] ERROR: could not read round_counter.txt\n";
            return 1;
        }
        int roundnum = 0;
        roundFile >> roundnum;
        roundFile.close();

        // Fetch aggregated encrypted params from server
        std::string url = "http://localhost:8000/s2c/agg_params?client_id=client1&round=" + std::to_string(roundnum);
        std::string response = HttpGetJson(url);

        // Log communication download size (response size in bytes) to CSV (no headers)
        size_t response_size = response.size();
        std::ofstream logFile("client1_data/comm_logs.csv", std::ios_base::app);
        logFile << roundnum << ",client1,download," << response_size << "\n";
        logFile.close();

        json resp_json = json::parse(response);

        if (!resp_json.contains("metadata")) {
            std::cerr << "[c1_decrypt] ERROR: metadata missing in server response\n";
            return 1;
        }
        if (!resp_json.contains("data")) {
            std::cerr << "[c1_decrypt] ERROR: data missing in server response\n";
            return 1;
        }

        json metadata = resp_json["metadata"];
        json data = resp_json["data"];

        std::string b64_params;
        if (data.contains("params")) {
            b64_params = data["params"];
        } else if (data.contains("agg_params")) {
            b64_params = data["agg_params"];
        } else {
            std::cerr << "[c1_decrypt] ERROR: neither 'params' nor 'agg_params' present in data\n";
            return 1;
        }

        
        if (!data.contains("chunk_counts")) {
            std::cerr << "[c1_decrypt] ERROR: chunk_counts missing in data\n";
            return 1;
        }
        if (!data.contains("orig_sizes")) {
            std::cerr << "[c1_decrypt] ERROR: orig_sizes missing in data\n";
            return 1;
        }

        std::vector<size_t> chunk_counts = data["chunk_counts"].get<std::vector<size_t>>();
        std::vector<size_t> orig_sizes = data["orig_sizes"].get<std::vector<size_t>>();

        // Decode Base64 to bytes
        std::vector<uint8_t> param_bytes = Base64Decode(b64_params);

        std::stringstream ss;
        ss.write(reinterpret_cast<const char*>(param_bytes.data()), param_bytes.size());

        // Deserialize vector of ciphertexts (chunks)
        std::vector<Ciphertext<DCRTPoly>> ciphertexts;
        Serial::Deserialize(ciphertexts, ss, SerType::BINARY);

        // Sanity check: total ciphertexts should equal sum of chunk_counts
        size_t total_chunks = 0;
        for (size_t cnt : chunk_counts) total_chunks += cnt;

        if (total_chunks != ciphertexts.size()) {
            std::cerr << "[c1_decrypt] ERROR: mismatch between chunk_counts sum (" << total_chunks 
                      << ") and ciphertexts size (" << ciphertexts.size() << ")\n";
            return 1;
        }
        if (chunk_counts.size() != orig_sizes.size()) {
            std::cerr << "[c1_decrypt] ERROR: chunk_counts and orig_sizes size mismatch\n";
            return 1;
        }

        // Decrypt and reconstruct original arrays by concatenating each array's chunk plaintexts
        json out_json = json::array();
        size_t chunk_index = 0;

        for (size_t layer_idx = 0; layer_idx < chunk_counts.size(); ++layer_idx) {
            size_t count = chunk_counts[layer_idx];
            size_t orig_size = orig_sizes[layer_idx];

            std::vector<double> full_array;

            for (size_t i = 0; i < count; ++i, ++chunk_index) {
                Plaintext pt;
                auto decrypt_result = cc->Decrypt(privKey, ciphertexts[chunk_index], &pt);
                if (!decrypt_result.isValid) {
                    std::cerr << "[c1_decrypt] ERROR: Decryption failed for ciphertext index " << chunk_index << std::endl;
                    return 1;
                }

                pt->SetLength(pt->GetLength());
                const auto& vals = pt->GetCKKSPackedValue();

                for (const auto& val : vals) {
                    full_array.push_back(val.real());
                }
            }

            // Trim any trailing padded zeros before saving
            if (full_array.size() > orig_size) {
                full_array.resize(orig_size);
            }

            // Convert concatenated and trimmed array to JSON array
            json arr_json = json::array();
            for (double v : full_array) {
                arr_json.push_back(v);
            }
            out_json.push_back(arr_json);
        }

        // Save decrypted aggregated weights for warm start
        std::ofstream outFile("client1_data/agg_wc.json");
        if (!outFile) {
            std::cerr << "[c1_decrypt] ERROR: could not write agg_wc.json\n";
            return 1;
        }
        outFile << out_json.dump(4);
        outFile.close();

        std::cout << "[c1_decrypt] Saved decrypted aggregated weights to agg_wc.json" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[c1_decrypt] Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

