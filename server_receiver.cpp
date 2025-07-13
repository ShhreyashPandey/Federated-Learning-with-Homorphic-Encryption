#include "openfhe.h"
#include "cryptocontext-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

#include "serialization_utils.h"
#include "curl_utils.h"
#include "base64_utils.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include <complex>

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

vector<complex<double>> getModelWeights(size_t dim) {
    return vector<complex<double>>(dim, 0.5);  // Fixed weights
}

int main() {
    try {
        int round = getCurrentRound();
        cout << "[Server Receiver] \U0001f501 Using round " << round << endl;

        string url = "http://localhost:8000/encrypted-data?round=" + to_string(round);
        json data = json::parse(HttpGetJson(url));

        auto ciphers = data["ciphertexts"];
        auto rekeys = data["rekeys"];

        if (ciphers.size() != 2 || rekeys.size() != 2) {
            cerr << "[Server Receiver] Missing ciphertexts or rekeys" << endl;
            return 1;
        }

        // Load context
        CryptoContext<DCRTPoly> cc;
        ifstream ccIn("cc.bin", ios::binary);
        Serial::Deserialize(cc, ccIn, SerType::BINARY);

        // Deserialize eval keys
        for (const auto& c : ciphers) {
            vector<uint8_t> evm_bytes = Base64Decode(c["eval_mult_key"]);
            stringstream evm_stream(string(evm_bytes.begin(), evm_bytes.end()));
            cc->DeserializeEvalMultKey(evm_stream, SerType::BINARY);

            vector<uint8_t> evs_bytes = Base64Decode(c["eval_sum_key"]);
            stringstream evs_stream(string(evs_bytes.begin(), evs_bytes.end()));
            cc->DeserializeEvalSumKey(evs_stream, SerType::BINARY);
        }

        // Deserialize rekeys
        map<string, EvalKey<DCRTPoly>> rekeyMap;
        for (const auto& rk : rekeys) {
            string key = rk["rekey_from"].get<string>() + "_to_" + rk["rekey_to"].get<string>();
            vector<uint8_t> rk_bytes = Base64Decode(rk["rekey"]);
            stringstream rk_stream(string(rk_bytes.begin(), rk_bytes.end()));
            EvalKey<DCRTPoly> rk_obj;
            Serial::Deserialize(rk_obj, rk_stream, SerType::BINARY);
            rekeyMap[key] = rk_obj;
        }

        // Ciphertext loading
        Ciphertext<DCRTPoly> ct1 = DeserializeCiphertextFromBase64(ciphers[0]["ciphertext"]);
        Ciphertext<DCRTPoly> ct2 = DeserializeCiphertextFromBase64(ciphers[1]["ciphertext"]);

        // ReEncrypt to common keyspace (client1)
        Ciphertext<DCRTPoly> ct2_to_1 = cc->ReEncrypt(ct2, rekeyMap["client2_to_client1"]);

        // Homomorphic Ops
        auto sum = cc->EvalAdd(ct1, ct2_to_1);
        auto product = cc->EvalMult(ct1, ct2_to_1);
        cc->RescaleInPlace(product);

        // Simulate Model Inference
        Plaintext ptTmp;
        PrivateKey<DCRTPoly> sk;
        ifstream skStream("client1_private.key", ios::binary);
        Serial::Deserialize(sk, skStream, SerType::BINARY);
        cc->Decrypt(sk, ct1, &ptTmp);
        ptTmp->SetLength(ptTmp->GetLength());
        size_t dim = ptTmp->GetLength();

        auto weights = getModelWeights(dim);
        auto ptWeights = cc->MakeCKKSPackedPlaintext(weights);
        auto weighted = cc->EvalMult(ct1, ptWeights);
        cc->RescaleInPlace(weighted);
        auto inference = cc->EvalSum(weighted, dim);

        // Re-encrypt results for client2
        auto sum_for_c2 = cc->ReEncrypt(sum, rekeyMap["client1_to_client2"]);
        auto prod_for_c2 = cc->ReEncrypt(product, rekeyMap["client1_to_client2"]);
        auto inf_for_c2  = cc->ReEncrypt(inference, rekeyMap["client1_to_client2"]);

        // Save all results
        // For Client 1
        std::ofstream sumOut1("sum_client1.ct", std::ios::binary);
        Serial::Serialize(sum, sumOut1, SerType::BINARY);

        std::ofstream productOut1("product_client1.ct", std::ios::binary);
        Serial::Serialize(product, productOut1, SerType::BINARY);

        // For Client 2
        std::ofstream sumOut2("sum_client2.ct", std::ios::binary);
        Serial::Serialize(sum_for_c2, sumOut2, SerType::BINARY);

        std::ofstream productOut2("product_client2.ct", std::ios::binary);
        Serial::Serialize(prod_for_c2, productOut2, SerType::BINARY);


        cout << "[Server Receiver] Computation complete and encrypted results saved for both clients." << endl;
        cout << "-----------------------------------------------------\n";

    } catch (const std::exception& e) {
        cerr << "[Server Receiver] Error: " << e.what() << endl;
        cout << "-----------------------------------------------------\n";
        return 1;
    }

    return 0;
}
