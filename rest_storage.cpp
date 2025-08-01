#include "rest_storage.h"
#include <fstream>
#include <filesystem>
#include <iostream>

using namespace std;

/* Public Keys*/
void FederatedStorage::StorePublicKey(const string& client_id,
    const string& pubkey_b64,
    const string& eval_mult_b64,
    const string& eval_sum_b64) {

    lock_guard<mutex> lock(mtx_);
    public_keys_[client_id] = {
        {"public_key", pubkey_b64},
        {"eval_mult_key", eval_mult_b64},
        {"eval_sum_key", eval_sum_b64}
    };
}

json FederatedStorage::GetPublicKey(const string& client_id) {
    lock_guard<mutex> lock(mtx_);
    if (public_keys_.count(client_id))
        return public_keys_[client_id];
    return json();  // Empty response if not found
}

/* ReKey */
void FederatedStorage::StoreRekey(const string& from_id, const string& to_id, const string& rekey_b64) {
    lock_guard<mutex> lock(mtx_);
    rekeys_[from_id][to_id] = {
        {"from", from_id},
        {"to", to_id},
        {"rekey", rekey_b64}
    };
}

json FederatedStorage::GetRekey(const string& from_id, const string& to_id) {
    lock_guard<mutex> lock(mtx_);
    if (rekeys_.count(from_id) && rekeys_[from_id].count(to_id)) {
        return rekeys_[from_id][to_id];
    }
    return json();
}

/* Encrypted Parameters */
void FederatedStorage::StoreParams(const string& client_id, int round, const string& params_b64) {
    lock_guard<mutex> lock(mtx_);
    encrypted_params_[client_id][round] = params_b64;
}

json FederatedStorage::GetAllParams(int round) {
    lock_guard<mutex> lock(mtx_);
    json round_data;
    for (const auto& [client, rounds] : encrypted_params_) {
        if (rounds.count(round)) {
            round_data[client] = rounds.at(round);
        }
    }
    return round_data;
}

/* Aggregated Parameters */
void FederatedStorage::StoreAggregatedParams(int round, const json& aggregated_param_map) {
    lock_guard<mutex> lock(mtx_);
    aggregated_params_[round] = aggregated_param_map;
}

json FederatedStorage::GetAggregatedParam(const string& client_id, int round) {
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
void FederatedStorage::StoreResult(const string& client_id, int round, double accuracy, const string& model_name) {
    lock_guard<mutex> lock(mtx_);
    result_map_[round][client_id] = {
        {"client_id", client_id},
        {"round", round},
        {"accuracy", accuracy},
        {"model", model_name}
    };
}

json FederatedStorage::GetResult(const string& client_id, int round) {
    lock_guard<mutex> lock(mtx_);
    if (result_map_.count(round) && result_map_[round].count(client_id)) {
        return result_map_[round][client_id];
    }
    return json();
}

/* Log */
void FederatedStorage::LogRoundToFile(int round, const string& filepath) {
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

