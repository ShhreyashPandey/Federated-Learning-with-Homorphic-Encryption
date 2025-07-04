#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <filesystem>
#include <thread>
#include <map>

using namespace lbcrypto;
using namespace std;

void wait_for_file(const string& filename) {
    while (!filesystem::exists(filename)) {
        cout << "[Server Receiver] Waiting for file: " << filename << endl;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

// Parse metadata (model, version, round, etc.)
map<string, string> parseMetadata(const string& filename) {
    ifstream file(filename);
    string line;
    map<string, string> fields;

    while (getline(file, line)) {
        auto pos = line.find('=');
        if (pos == string::npos) continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        fields[key] = value;
    }
    return fields;
}

// Just a demo linear regression weight initializer
vector<complex<double>> getModelWeights(size_t dim) {
    return vector<complex<double>>(dim, 0.5); // fixed weights
}

size_t getInputDim(CryptoContext<DCRTPoly> cc, Ciphertext<DCRTPoly> ct, const PrivateKey<DCRTPoly>& sk) {
    Plaintext pt;
    cc->Decrypt(sk, ct, &pt);
    pt->SetLength(pt->GetLength());
    return pt->GetLength();
}

int main() {
    wait_for_file("model_input.txt");
    auto metadata = parseMetadata("model_input.txt");

    // Load all prerequisites
    wait_for_file("cc.bin");
    wait_for_file("ciphertext1.ct");
    wait_for_file("ciphertext2.ct");
    wait_for_file("rekey_1_to_2.key");
    wait_for_file("rekey_2_to_1.key");
    wait_for_file("eval_mult_1.key");
    wait_for_file("eval_mult_2.key");
    wait_for_file("eval_sum_1.key");
    wait_for_file("eval_sum_2.key");
    wait_for_file("client1_private.key");

    // Load CryptoContext
    CryptoContext<DCRTPoly> cc;
    ifstream ccIn("cc.bin", ios::binary);
    Serial::Deserialize(cc, ccIn, SerType::BINARY);

    // Load eval keys
    ifstream evmk1("eval_mult_1.key", ios::binary); cc->DeserializeEvalMultKey(evmk1, SerType::BINARY); evmk1.close();
    ifstream evmk2("eval_mult_2.key", ios::binary); cc->DeserializeEvalMultKey(evmk2, SerType::BINARY); evmk2.close();
    ifstream evks1("eval_sum_1.key", ios::binary); cc->DeserializeEvalSumKey(evks1, SerType::BINARY); evks1.close();
    ifstream evks2("eval_sum_2.key", ios::binary); cc->DeserializeEvalSumKey(evks2, SerType::BINARY); evks2.close();

    // Load ciphertexts
    Ciphertext<DCRTPoly> ct1, ct2;
    ifstream ct1In("ciphertext1.ct", ios::binary); Serial::Deserialize(ct1, ct1In, SerType::BINARY);
    ifstream ct2In("ciphertext2.ct", ios::binary); Serial::Deserialize(ct2, ct2In, SerType::BINARY);

    // Load rekey
    EvalKey<DCRTPoly> rk12, rk21;
    ifstream rk12In("rekey_1_to_2.key", ios::binary); Serial::Deserialize(rk12, rk12In, SerType::BINARY);
    ifstream rk21In("rekey_2_to_1.key", ios::binary); Serial::Deserialize(rk21, rk21In, SerType::BINARY);

    // Re-encrypt for cross-client operations
    auto ct1_to_2 = cc->ReEncrypt(ct1, rk12);
    auto ct2_to_1 = cc->ReEncrypt(ct2, rk21);

    // Add + Mult
    auto sum1 = cc->EvalAdd(ct1, ct2_to_1);
    auto prod1 = cc->EvalMult(ct1, ct2_to_1); cc->RescaleInPlace(prod1);
    auto sum2 = cc->EvalAdd(ct1_to_2, ct2);
    auto prod2 = cc->EvalMult(ct1_to_2, ct2); cc->RescaleInPlace(prod2);

    Serial::SerializeToFile("sum_to_1.ct", sum1, SerType::BINARY);
    Serial::SerializeToFile("product_to_1.ct", prod1, SerType::BINARY);
    Serial::SerializeToFile("sum.ct", sum2, SerType::BINARY);
    Serial::SerializeToFile("product.ct", prod2, SerType::BINARY);

    // Inference-like operation (simulate secure model application)
    PrivateKey<DCRTPoly> sk;
    ifstream skStream("client1_private.key", ios::binary); Serial::Deserialize(sk, skStream, SerType::BINARY);

    size_t dim = getInputDim(cc, ct1, sk);
    auto weights = getModelWeights(dim);
    Plaintext ptWeights = cc->MakeCKKSPackedPlaintext(weights);
    ptWeights->SetLength(dim);

    auto ctWeighted = cc->EvalMult(ct1, ptWeights);
    cc->RescaleInPlace(ctWeighted);
    auto ctSum = cc->EvalSum(ctWeighted, dim);

    // Save the final encrypted output
    ofstream out("encrypted_aggregated_weights.ct", ios::binary);
    Serial::Serialize(ctSum, out, SerType::BINARY);
    out.close();

    // Write accompanying metadata
    ofstream metaOut("encrypted_aggregated_weights.txt");
    for (const auto& [key, value] : metadata) {
        metaOut << key << "=" << value << endl;
    }
    metaOut << "encrypted_weights=encrypted_aggregated_weights.ct" << endl;
    metaOut << "direction=server_to_client" << endl;
    metaOut.close();

    cout << "[Server Receiver] ✅ Computation complete. Result saved.\n";
    return 0;
}
