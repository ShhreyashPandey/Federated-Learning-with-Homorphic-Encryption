#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

#include "serialization_utils.h"
#include "curl_utils.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <map>
#include <vector>

using namespace lbcrypto;
using json = nlohmann::json;
using namespace std;

// Load base64-encoded ciphertext from binary file
string loadCiphertextAsBase64(const string& filename) {
    Ciphertext<DCRTPoly> ct;
    ifstream in(filename, ios::binary);
    if (!in.is_open()) throw runtime_error("Failed to open " + filename);
    Serial::Deserialize(ct, in, SerType::BINARY);
    return SerializeCiphertextToBase64(ct);
}

// Helper to POST one result payload to server
void postClientResult(int round, const string& clientId,
                      const string& ct_sum_b64,
                      const string& ct_prod_b64) {

    json payload = {
        {"round", round},
        {"client_id", clientId},
        {"results", {
            {"sum", ct_sum_b64},
            {"product", ct_prod_b64}
        }},
        {"model", "LinearRegression"},
        {"version", "1.0"},
        {"direction", "server_to_client"}
    };

    string response = HttpPostJson("http://localhost:8000/result", payload.dump());
    cout << "[Server Sender] Result posted for " << clientId << ": " << response << endl;
}

int getCurrentRound() {
    ifstream file("round_counter.txt");
    if (!file.is_open()) throw runtime_error("Cannot open round_counter.txt");
    int round;
    file >> round;
    return round;
}

int main() {
    try {
        int round = getCurrentRound();
        cout << "[Server Sender] Using round " << round << endl;

        // Paths to encrypted files
        map<string, string> files = {
            {"client1_sum",        "sum_client1.ct"},
            {"client1_product",    "product_client1.ct"},
            {"client2_sum",        "sum_client2.ct"},
            {"client2_product",    "product_client2.ct"}
        };

        // Load base64 ciphertexts
        map<string, string> base64s;
        for (const auto& [key, path] : files) {
            base64s[key] = loadCiphertextAsBase64(path);
        }

        // Post results for client1
        postClientResult(round, "client1",
                         base64s["client1_sum"],
                         base64s["client1_product"]);

        // Post results for client2
        postClientResult(round, "client2",
                         base64s["client2_sum"],
                         base64s["client2_product"]);

        cout << "-----------------------------------------------------\n";
    } catch (const std::exception& e) {
        cerr << "[Server Sender] Error: " << e.what() << endl;
        cout << "-----------------------------------------------------\n";
        return 1;
    }

    return 0;
}
