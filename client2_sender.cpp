#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

#include "serialization_utils.h"
#include "curl_utils.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>
#include <complex>
#include <fstream>
#include <limits>

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
    //STEP 0: Read Round Info
    int round = getCurrentRound();
    cout << "[Client2 Sender] Using round " << round << endl;

    // STEP 1: Get Input Vector
    string url = "http://localhost:8000/input?round=" + to_string(round);
    string raw = HttpGetJson(url);
    if (raw.empty()) {
        cerr << "Empty response from GET /input" << endl;
        return 1;
    }

    json model_input;
    try {
        model_input = json::parse(raw);
    } catch (const json::parse_error& e) {
        cerr << "Failed to parse JSON from /input: " << e.what() << endl;
        return 1;
    }

    cout << "[Debug] Full model_input JSON:\n" << model_input.dump(4) << endl;

    if (!model_input.contains("input_vector") || !model_input["input_vector"].is_array()) {
        cerr << "input_vector is missing or not an array in input JSON" << endl;
        return 1;
    }

    //STEP 2: Load CryptoContext
    CryptoContext<DCRTPoly> cc;
    ifstream ccIn("cc.bin", ios::binary);
    if (!ccIn.is_open()) {
        cerr << "Failed to open cc.bin" << endl;
        return 1;
    }
    Serial::Deserialize(cc, ccIn, SerType::BINARY);
    if (!cc || cc->GetEncodingParams() == nullptr) {
        cerr << "CryptoContext is invalid after deserialization" << endl;
        return 1;
    }

    size_t batchSize = cc->GetEncodingParams()->GetBatchSize();
    cout << "[Debug] CKKS Batch Size = " << batchSize << endl;

    // STEP 3: Validate and Parse Input
    vector<complex<double>> input;
    input.reserve(model_input["input_vector"].size());

    for (const auto& val : model_input["input_vector"]) {
        if (!val.is_number()) {
            cerr << "input_vector contains non-numeric value." << endl;
            return 1;
        }
        input.emplace_back(val.get<double>());
    }

    size_t inputSize = input.size();
    cout << "[Debug] Parsed input_vector of size " << inputSize << ": [ ";
    for (const auto& v : input) cout << v.real() << " ";
    cout << "]" << endl;

    if (inputSize == 0 || inputSize > batchSize) {
        cerr << "Invalid input_vector size: " << inputSize << " (Batch size = " << batchSize << ")" << endl;
        return 1;
    }

    // STEP 4: Key Generation
    auto keyPair = cc->KeyGen();
    cc->EvalMultKeyGen(keyPair.secretKey);
    cc->EvalSumKeyGen(keyPair.secretKey);
    
    ofstream skOut("client2_private.key", ios::binary);
    if (!skOut.is_open()) {
        cerr << "Failed to open file to save private key" << endl;
        return 1;
    }
    Serial::Serialize(keyPair.secretKey, skOut, SerType::BINARY);
    skOut.close();
    cout << "[Client2 Sender] Private key saved to client2_private.key" << endl;


    // STEP 5: Encrypt Input
    Plaintext pt;
    Ciphertext<DCRTPoly> ct;

    try {
        pt = cc->MakeCKKSPackedPlaintext(input);
        pt->SetLength(inputSize);
        ct = cc->Encrypt(keyPair.publicKey, pt);
        cout << "[Debug] Encryption complete" << endl;
    } catch (const exception& e) {
        cerr << "[Error] During encryption: " << e.what() << endl;
        return 1;
    }

    // STEP 6: Serialize and POST
    string ct_b64, pk_b64, evm_b64, evs_b64;
    try {
        ct_b64 = SerializeCiphertextToBase64(ct);
        cout << "[Debug] Ciphertext base64 size: " << ct_b64.size() << " bytes" << endl;

        pk_b64 = SerializePublicKeyToBase64(keyPair.publicKey);
        evm_b64 = SerializeEvalMultKeyToBase64(cc);
        evs_b64 = SerializeEvalSumKeyToBase64(cc);
    } catch (const exception& e) {
        cerr << "[Error] During serialization: " << e.what() << endl;
        return 1;
    }

    json payload = {
        {"client_id", "client2"},
        {"round", round},
        {"ciphertext", ct_b64},
        {"public_key", pk_b64},
        {"eval_mult_key", evm_b64},
        {"eval_sum_key", evs_b64}
    };

    try {
        string response = HttpPostJson("http://localhost:8000/encrypted-data/ciphertext", payload.dump());
        cout << "[Client2 Sender] POST response: " << response << endl;
        cout << "-----------------------------------------------------\n";
    } catch (const exception& e) {
        cerr << "[Error] HTTP POST failed: " << e.what() << endl;
        cout << "-----------------------------------------------------\n";
        return 1;
    }

    return 0;
}
