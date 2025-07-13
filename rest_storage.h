#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct CiphertextEntry {
    std::string client_id;
    std::string ciphertext_b64;
    std::string public_key_b64;
    std::string eval_mult_key_b64;
    std::string eval_sum_key_b64;
};

struct ReKeyEntry {
    std::string rekey_from;
    std::string rekey_to;
    std::string rekey_b64;
};

class FederatedStorage {
public:
    void StoreInput(int round, const json& input);
    void AddCiphertext(int round, const CiphertextEntry& entry);
    void AddReKey(int round, const ReKeyEntry& entry);
    void StoreResult(int round, const std::string& client_id, const json& result);

    json GetInput(int round);
    std::vector<CiphertextEntry> GetCiphertexts(int round);
    std::vector<ReKeyEntry> GetRekeys(int round);
    json GetResult(int round, const std::string& client_id);

    void LogRoundToFile(int round, const std::string& filepath);

private:
    std::unordered_map<int, json> input_map_;
    std::unordered_map<int, std::vector<CiphertextEntry>> ciphertext_map_;
    std::unordered_map<int, std::vector<ReKeyEntry>> rekey_map_;
    std::unordered_map<int, std::unordered_map<std::string, json>> result_map_;
    std::mutex mtx_;
};
