#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

using namespace lbcrypto;
using namespace std;

void wait_for_file(const string& filename) {
    while (!filesystem::exists(filename)) {
        cout << "[Client1 Rekey] Waiting for file: " << filename << endl;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

int main() {
    wait_for_file("cc.bin");
    wait_for_file("client1_private.key");
    wait_for_file("client2_public.key");

    CryptoContext<DCRTPoly> cc;
    ifstream ccIn("cc.bin", ios::binary);
    Serial::Deserialize(cc, ccIn, SerType::BINARY);

    PrivateKey<DCRTPoly> sk1;
    ifstream skIn("client1_private.key", ios::binary);
    Serial::Deserialize(sk1, skIn, SerType::BINARY);

    PublicKey<DCRTPoly> pk2;
    ifstream pkIn("client2_public.key", ios::binary);
    Serial::Deserialize(pk2, pkIn, SerType::BINARY);

    auto rekey = cc->ReKeyGen(sk1, pk2);
    ofstream rkOut("rekey_1_to_2.key", ios::binary);
    Serial::Serialize(rekey, rkOut, SerType::BINARY);

    cout << "[Client1 Rekey] Re-encryption key generated ✅" << endl;
    return 0;
}
