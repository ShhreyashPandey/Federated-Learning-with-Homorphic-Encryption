#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <map>

using namespace lbcrypto;
using namespace std;

void wait_for_file(const string& filename) {
    while (!filesystem::exists(filename)) {
        cout << "[Client1 Receiver] Waiting for file: " << filename << endl;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

map<string, string> parseModelInput(const string& filename) {
    ifstream file(filename);
    string line;
    map<string, string> fields;

    while (getline(file, line)) {
        auto pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            fields[key] = value;
        }
    }
    return fields;
}

int main() {
    wait_for_file("cc.bin");
    wait_for_file("client1_private.key");
    wait_for_file("sum_to_1.ct");
    wait_for_file("product_to_1.ct");

    CryptoContext<DCRTPoly> cc;
    ifstream ccStream("cc.bin", ios::binary);
    Serial::Deserialize(cc, ccStream, SerType::BINARY);

    PrivateKey<DCRTPoly> sk;
    ifstream skStream("client1_private.key", ios::binary);
    Serial::Deserialize(sk, skStream, SerType::BINARY);

    Ciphertext<DCRTPoly> ctSum, ctProd;
    ifstream sumStream("sum_to_1.ct", ios::binary);
    Serial::Deserialize(ctSum, sumStream, SerType::BINARY);

    ifstream prodStream("product_to_1.ct", ios::binary);
    Serial::Deserialize(ctProd, prodStream, SerType::BINARY);

    Plaintext ptSum, ptProd;
    cc->Decrypt(sk, ctSum, &ptSum);
    ptSum->SetLength(1);
    cout << "🔓 Client 1 decrypted SUM: " << ptSum << endl;

    cc->Decrypt(sk, ctProd, &ptProd);
    ptProd->SetLength(1);
    cout << "🔓 Client 1 decrypted PRODUCT: " << ptProd << endl;

    // 🔄 Parse metadata
    wait_for_file("model_input.txt");
    auto meta = parseModelInput("model_input.txt");

    cout << "\n📦 [Client1 Receiver] Model: " << meta["model"]
         << "\n📦 Version: " << meta["version"]
         << "\n📦 Round: " << meta["round"]
         << "\n📥 Output: " << meta["output"]
         << "\n🔁 Direction: " << meta["direction"] << "\n"
         << endl;

    return 0;
}
