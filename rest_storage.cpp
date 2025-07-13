#include "rest_storage.h"
#include <fstream>
#include <filesystem>

void FederatedStorage::StoreInput(int round, const json& input) {
    std::lock_guard<std::mutex> lock(mtx_);
    input_map_[round] = input;
}

void FederatedStorage::AddCiphertext(int round, const CiphertextEntry& entry) {
    std::lock_guard<std::mutex> lock(mtx_);
    ciphertext_map_[round].push_back(entry);
}

void FederatedStorage::AddReKey(int round, const ReKeyEntry& entry) {
    std::lock_guard<std::mutex> lock(mtx_);
    rekey_map_[round].push_back(entry);
}

void FederatedStorage::StoreResult(int round, const std::string& client_id, const json& result) {
    std::lock_guard<std::mutex> lock(mtx_);
    result_map_[round][client_id] = result;
}

json FederatedStorage::GetInput(int round) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (input_map_.count(round))
        return input_map_[round];
    return json::object();
}

std::vector<CiphertextEntry> FederatedStorage::GetCiphertexts(int round) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (ciphertext_map_.count(round))
        return ciphertext_map_[round];
    return {};
}

std::vector<ReKeyEntry> FederatedStorage::GetRekeys(int round) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (rekey_map_.count(round))
        return rekey_map_[round];
    return {};
}

json FederatedStorage::GetResult(int round, const std::string& client_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (result_map_.count(round) && result_map_[round].count(client_id))
        return result_map_[round][client_id];
    return json::object();
}

void FederatedStorage::LogRoundToFile(int round, const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::filesystem::create_directories("logs");
    std::ofstream out(filepath);
    if (!out.is_open()) return;

    out << input_map_[round].dump(4) << "\n";
    for (const auto& [client_id, result] : result_map_[round]) {
        out << ("Client: " + client_id) << "\n";
        out << result.dump(4) << "\n";
    }
    out.close();
}
