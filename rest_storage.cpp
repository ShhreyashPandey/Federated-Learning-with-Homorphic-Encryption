#include "rest_storage.h"
#include <fstream>
#include <filesystem>
#include <iostream>

using namespace std;

/* Public Keys */
void FederatedStorage::StorePublicKey(const string& client_id,
                                      const string& pubkey_b64,
                                      const string& eval_mult_b64,
                                      const string& eval_sum_b64) 
{
    lock_guard<mutex> lock(mtx_);
    public_keys_[client_id] = {
        {"public_key", pubkey_b64},
        {"eval_mult_key", eval_mult_b64},
        {"eval_sum_key", eval_sum_b64}
    };
}

json FederatedStorage::GetPublicKey(const string& client_id) 
{
    lock_guard<mutex> lock(mtx_);
    if (public_keys_.count(client_id))
        return public_keys_[client_id];
    return json();  // Empty response if not found
}

/* ReKey */
void FederatedStorage::StoreRekey(const string& from_id, const string& to_id, const string& rekey_b64) 
{
    lock_guard<mutex> lock(mtx_);
    rekeys_[from_id][to_id] = {
        {"from", from_id},
        {"to", to_id},
        {"rekey", rekey_b64}
    };
}

json FederatedStorage::GetRekey(const string& from_id, const string& to_id) 
{
    lock_guard<mutex> lock(mtx_);
    if (rekeys_.count(from_id) && rekeys_[from_id].count(to_id)) {
        return rekeys_[from_id][to_id];
    }
    return json();
}

/* Encrypted Parameters (Base64 serialized ciphertext vector string) */
void FederatedStorage::StoreParams(const std::string& client_id, int round, const std::string& params_b64, const std::vector<size_t>& chunk_counts) {
    // Overload to accept orig_sizes optional parameter
    StoreParams(client_id, round, params_b64, chunk_counts, {});
}

void FederatedStorage::StoreParams(const std::string& client_id, int round, const std::string& params_b64, const std::vector<size_t>& chunk_counts, const std::vector<size_t>& orig_sizes) {
    std::lock_guard<std::mutex> lock(mtx_);
    encrypted_params_[client_id][round] = params_b64;
    
    if (!chunk_counts.empty()) {
        chunk_counts_[client_id][round] = chunk_counts;
    }

    if (!orig_sizes.empty()) {
        orig_sizes_[client_id][round] = orig_sizes;
    }
}

std::vector<size_t> FederatedStorage::GetChunkCounts(const std::string& client_id, int round) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (chunk_counts_.count(client_id) && chunk_counts_[client_id].count(round)) {
        return chunk_counts_[client_id][round];
    }
    return {};
}

std::vector<size_t> FederatedStorage::GetOrigSizes(const std::string& client_id, int round) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (orig_sizes_.count(client_id) && orig_sizes_[client_id].count(round)) {
        return orig_sizes_[client_id][round];
    }
    return {};
}

json FederatedStorage::GetAllParams(int round) 
{
    lock_guard<mutex> lock(mtx_);
    json round_data = json::object();
    for (const auto& [client, rounds] : encrypted_params_) {
        if (rounds.count(round)) {
            round_data[client] = rounds.at(round);
        }
    }
    return round_data;
}

/* Aggregated Parameters (Base64 serialized ciphertext vector string) */
void FederatedStorage::StoreAggregatedParams(int round, const json& aggregated_param_map) 
{
    lock_guard<mutex> lock(mtx_);
    aggregated_params_[round] = aggregated_param_map;
}

json FederatedStorage::GetAggregatedParam(const string& client_id, int round) 
{
    lock_guard<mutex> lock(mtx_);
    if (aggregated_params_.count(round) && aggregated_params_[round].contains(client_id)) {
        return {
            {"client_id", client_id},
            {"round", round},
            {"agg_params", aggregated_params_[round][client_id]}
        };
    }
    return json();
}

/* Result */
void FederatedStorage::StoreResult(const string& client_id, int round, double accuracy, const string& model_name) 
{
    lock_guard<mutex> lock(mtx_);
    result_map_[round][client_id] = {
        {"client_id", client_id},
        {"round", round},
        {"accuracy", accuracy},
        {"model", model_name}
    };
}

json FederatedStorage::GetResult(const string& client_id, int round) 
{
    lock_guard<mutex> lock(mtx_);
    if (result_map_.count(round) && result_map_[round].count(client_id)) {
        return result_map_[round][client_id];
    }
    return json();
}

/* Log */
void FederatedStorage::LogRoundToFile(int round, const string& filepath) 
{
    lock_guard<mutex> lock(mtx_);
    filesystem::create_directories("logs");
    ofstream out(filepath);
    if (!out.is_open()) return;

    out << "Round: " << round << "\n\n";

    if (aggregated_params_.count(round)) {
        out << "[Aggregated Params]\n" << aggregated_params_[round].dump(4) << "\n\n";
    }
    if (result_map_.count(round)) {
        for (const auto& [client, result] : result_map_[round]) {
            out << "[Result for Client: " << client << "]\n" << result.dump(4) << "\n";
        }
    }
    out.close();
}

