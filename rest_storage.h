#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class FederatedStorage {
public:

    // Public Key & Eval Keys
    void StorePublicKey(const std::string& client_id,
                        const std::string& pubkey_b64,
                        const std::string& eval_mult_b64,
                        const std::string& eval_sum_b64);
    json GetPublicKey(const std::string& client_id);

    // ReEncryption Key
    void StoreRekey(const std::string& from_id, const std::string& to_id, const std::string& rekey_b64);
    json GetRekey(const std::string& from_id, const std::string& to_id);

    // Encrypted Parameters (Base64 string of serialized ciphertext vector) stored by client and round
    void StoreParams(const std::string& client_id, int round, const std::string& params_b64, const std::vector<size_t>& chunk_counts = {});
    void StoreParams(const std::string& client_id, int round, const std::string& params_b64, const std::vector<size_t>& chunk_counts, const std::vector<size_t>& orig_sizes);
    json GetAllParams(int round);

    // Retrieve chunk counts for params
    std::vector<size_t> GetChunkCounts(const std::string& client_id, int round);

    // Retrieve original sizes for params
    std::vector<size_t> GetOrigSizes(const std::string& client_id, int round);  // << New

    // Aggregated Encrypted Parameters (Base64 string) mapping client_id → base64
    void StoreAggregatedParams(int round, const json& aggregated_param_map); // { "client1": "base64", ... }
    json GetAggregatedParam(const std::string& client_id, int round);

    // Results / Accuracy information
    void StoreResult(const std::string& client_id, int round, double accuracy, const std::string& model_name);
    json GetResult(const std::string& client_id, int round);

    void LogRoundToFile(int round, const std::string& filepath);

private:
    std::mutex mtx_;

    std::unordered_map<std::string, json> public_keys_;  // client_id → { pub, eval_mult, eval_sum }
    std::unordered_map<std::string, std::unordered_map<std::string, json>> rekeys_; // from→to→{...}
    std::unordered_map<std::string, std::unordered_map<int, std::string>> encrypted_params_; // client_id → round → base64 param
    
    // Map to store chunk counts metadata: client_id → round → chunkCounts vector
    std::unordered_map<std::string, std::unordered_map<int, std::vector<size_t>>> chunk_counts_;
    
    // Map to store original sizes metadata: client_id → round → origSizes vector
    std::unordered_map<std::string, std::unordered_map<int, std::vector<size_t>>> orig_sizes_;  // << New
    
    std::unordered_map<int, json> aggregated_params_;  // round → { client_id: b64 }

    std::unordered_map<int, std::unordered_map<std::string, json>> result_map_;  // round → client_id → result JSON
};

