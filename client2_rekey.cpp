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
        cout << "[Client2 Rekey] Waiting for file: " << filename << endl;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

int main() {
    wait_for_file("cc.bin");
    wait_for_file("client2_private.key");
    wait_for_file("client1_public.key");

    CryptoContext<DCRTPoly> cc;
    ifstream ccIn("cc.bin", ios::binary);
    Serial::Deserialize(cc, ccIn, SerType::BINARY);

    PrivateKey<DCRTPoly> sk2;
    ifstream skIn("client2_private.key", ios::binary);
    Serial::Deserialize(sk2, skIn, SerType::BINARY);

    PublicKey<DCRTPoly> pk1;
    ifstream pkIn("client1_public.key", ios::binary);
    Serial::Deserialize(pk1, pkIn, SerType::BINARY);

    auto rekey = cc->ReKeyGen(sk2, pk1);
    ofstream rkOut("rekey_2_to_1.key", ios::binary);
    Serial::Serialize(rekey, rkOut, SerType::BINARY);

    cout << "[Client2 Rekey] Re-encryption key generated ✅" << endl;
    return 0;
}
