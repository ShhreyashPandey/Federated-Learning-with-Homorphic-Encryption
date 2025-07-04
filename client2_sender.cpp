#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <filesystem>
#include <thread>
#include <sstream>
#include <map>

using namespace lbcrypto;
using namespace std;

void wait_for_file(const string& filename) {
    while (!filesystem::exists(filename)) {
        cout << "[Client2] Waiting for file: " << filename << endl;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

map<string, string> parseModelInput(const string& filename, vector<complex<double>>& input) {
    ifstream file(filename);
    string line;
    map<string, string> fields;

    while (getline(file, line)) {
        auto pos = line.find('=');
        if (pos == string::npos) continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        if (key == "input_vector") {
            stringstream ss(value);
            string val;
            while (getline(ss, val, ',')) {
                input.push_back(stod(val));
            }
        } else {
            fields[key] = value;
        }
    }
    return fields;
}

int main() {
    wait_for_file("model_input.txt");

    vector<complex<double>> input;
    map<string, string> metadata = parseModelInput("model_input.txt", input);

    if (input.empty()) {
        cerr << "[Client2 Sender] No valid input_vector found in model_input.txt ❌" << endl;
        return 1;
    }

    CryptoContext<DCRTPoly> cc;
    ifstream ccIn("cc.bin", ios::binary);
    Serial::Deserialize(cc, ccIn, SerType::BINARY);

    auto keyPair = cc->KeyGen();
    cc->EvalMultKeyGen(keyPair.secretKey);
    cc->EvalSumKeyGen(keyPair.secretKey);

    // Save public/private keys
    ofstream pubOut("client2_public.key", ios::binary);
    Serial::Serialize(keyPair.publicKey, pubOut, SerType::BINARY);
    pubOut.close();

    ofstream privOut("client2_private.key", ios::binary);
    Serial::Serialize(keyPair.secretKey, privOut, SerType::BINARY);
    privOut.close();

    // Save EvalMult/EvalSum Keys
    ofstream evmkOut("eval_mult_2.key", ios::binary);
    cc->SerializeEvalMultKey(evmkOut, SerType::BINARY, "");
    evmkOut.close();

    ofstream evksOut("eval_sum_2.key", ios::binary);
    cc->SerializeEvalSumKey(evksOut, SerType::BINARY);
    evksOut.close();

    // Encrypt input vector
    Plaintext pt = cc->MakeCKKSPackedPlaintext(input);
    pt->SetLength(input.size());
    auto ct = cc->Encrypt(keyPair.publicKey, pt);

    // Save ciphertext
    ofstream ctOut("ciphertext2.ct", ios::binary);
    Serial::Serialize(ct, ctOut, SerType::BINARY);
    ctOut.close();

    // ✅ Write metadata file for server
    ofstream metaOut("client2_to_server.txt");
    if (!metaOut.is_open()) {
        cerr << "[Client2 Sender] Failed to write client2_to_server.txt ❌" << endl;
        return 1;
    }

    for (const auto& [key, value] : metadata) {
        metaOut << key << "=" << value << endl;
    }
    metaOut << "ciphertext=ciphertext2.ct" << endl;

    metaOut.close();

    cout << "[Client2 Sender] ✅ Encrypted input_vector and wrote metadata" << endl;
    return 0;
}
