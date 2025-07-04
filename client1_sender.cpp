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
        cout << "[Client1] Waiting for file: " << filename << endl;
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
        cerr << "[Client1 Sender] No valid input_vector found in model_input.txt ❌" << endl;
        return 1;
    }

    CryptoContext<DCRTPoly> cc;
    ifstream ccIn("cc.bin", ios::binary);
    Serial::Deserialize(cc, ccIn, SerType::BINARY);

    auto keyPair = cc->KeyGen();
    cc->EvalMultKeyGen(keyPair.secretKey);
    cc->EvalSumKeyGen(keyPair.secretKey);

    // Save keys
    ofstream pubOut("client1_public.key", ios::binary);
    Serial::Serialize(keyPair.publicKey, pubOut, SerType::BINARY);
    pubOut.close();

    ofstream privOut("client1_private.key", ios::binary);
    Serial::Serialize(keyPair.secretKey, privOut, SerType::BINARY);
    privOut.close();

    // Save EvalMultKey
    ofstream evmkOut("eval_mult_1.key", ios::binary);
    cc->SerializeEvalMultKey(evmkOut, SerType::BINARY, "");
    evmkOut.close();

    // Save EvalSumKey
    ofstream evksOut("eval_sum_1.key", ios::binary);
    cc->SerializeEvalSumKey(evksOut, SerType::BINARY);
    evksOut.close();

    // Encrypt input vector
    Plaintext pt = cc->MakeCKKSPackedPlaintext(input);
    pt->SetLength(input.size());
    auto ct = cc->Encrypt(keyPair.publicKey, pt);

    // Serialize ciphertext to binary file
    ofstream ctOut("ciphertext1.ct", ios::binary);
    Serial::Serialize(ct, ctOut, SerType::BINARY);
    ctOut.close();

    // ✅ Write metadata + ciphertext location to text file for server
    ofstream metaOut("client1_to_server.txt");
    if (!metaOut.is_open()) {
        cerr << "[Client1 Sender] Failed to write client1_to_server.txt ❌" << endl;
        return 1;
    }

    for (const auto& [key, value] : metadata) {
        metaOut << key << "=" << value << endl;
    }
    metaOut << "ciphertext=ciphertext1.ct" << endl;

    metaOut.close();

    cout << "[Client1 Sender] ✅ Encrypted input_vector and wrote metadata" << endl;
    return 0;
}
