#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

#include "serialization_utils.h"
#include "curl_utils.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace lbcrypto;
using json = nlohmann::json;
using namespace std;

int getCurrentRound() {
    ifstream file("round_counter.txt");
    if (!file.is_open()) throw runtime_error("Failed to open round_counter.txt");
    int round;
    file >> round;
    file.close();
    if (round <= 0) throw runtime_error("Invalid round number in round_counter.txt");
    return round;
}

int main() {
    try {
        int round = getCurrentRound();
        cout << "[Client1 Rekey] Using round " << round << endl;

        // STEP 1: Load CryptoContext
        cout << "[Client1 Rekey] Loading CryptoContext..." << endl;
        CryptoContext<DCRTPoly> cc;
        ifstream ccIn("cc.bin", ios::binary);
        if (!ccIn.is_open()) {
            throw runtime_error("Failed to open cc.bin");
        }
        Serial::Deserialize(cc, ccIn, SerType::BINARY);
        cout << "[Client1 Rekey] CryptoContext loaded" << endl;

        // STEP 2: Load private key
        cout << "[Client1 Rekey] Loading private key..." << endl;
        PrivateKey<DCRTPoly> sk;
        ifstream skIn("client1_private.key", ios::binary);
        if (!skIn.is_open()) {
            throw runtime_error("Failed to open client1_private.key");
        }
        Serial::Deserialize(sk, skIn, SerType::BINARY);
        cout << "[Client1 Rekey] Private key loaded" << endl;

        // STEP 3: Fetch 
        cout << "[Client1 Rekey] Fetching ciphertext metadata from server..." << endl;
        string url = "http://localhost:8000/encrypted-data?round=" + to_string(round);
        string responseStr = HttpGetJson(url);
        cout << "[Client1 Rekey] Raw JSON size: " << responseStr.size() << " bytes" << endl;

        json response;
        try {
            response = json::parse(responseStr);
        } catch (const std::exception& e) {
            throw runtime_error(string("Failed to parse JSON from server: ") + e.what());
        }
        cout << "[Client1 Rekey] JSON parsed" << endl;

        // STEP 4: Extract public key
        bool found = false;
        PublicKey<DCRTPoly> pk2;

        if (!response.contains("ciphertexts") || !response["ciphertexts"].is_array()) {
            throw runtime_error("Missing or invalid 'ciphertexts' field in server response.");
        }

        for (const auto& ct : response["ciphertexts"]) {
            if (!ct.contains("client_id") || !ct.contains("public_key")) {
                continue;
            }

            string clientId = ct["client_id"].get<string>();
            if (clientId == "client2") {
                string pk_b64 = ct["public_key"].get<string>();
                cout << "[Client1 Rekey] Found client2 public key. Base64 size: " << pk_b64.size() << " bytes" << endl;

                if (pk_b64.empty()) {
                    throw runtime_error("Client2 public key is empty.");
                }

                try {
                    pk2 = DeserializePublicKeyFromBase64(pk_b64);
                } catch (const std::exception& ex) {
                    cerr << "Failed to deserialize client2 public key: " << ex.what() << endl;
                    throw;
                }

                found = true;
                break;
            }
        }


        if (!found) throw runtime_error("Public key for client2 not found");

        // STEP 5: Generate and POST rekey
        cout << "[Client1 Rekey] Generating ReKey..." << endl;
        auto rekey = cc->ReKeyGen(sk, pk2);
        string rk_b64 = SerializeEvalKeyToBase64(rekey);

        json payload = {
            {"rekey_from", "client1"},
            {"rekey_to", "client2"},
            {"round", round},
            {"rekey", rk_b64}
        };

        string postResp = HttpPostJson("http://localhost:8000/encrypted-data/rekey", payload.dump());
        cout << "[Client1 Rekey] Posted rekey to server: " << postResp << endl;
        cout << "-----------------------------------------------------\n";

    } catch (const std::exception& e) {
        cerr << "[Client1 Rekey] Error: " << e.what() << endl;
        cout << "-----------------------------------------------------\n";
        return 1;
    }

    return 0;
}
