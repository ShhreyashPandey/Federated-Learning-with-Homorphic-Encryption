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

void decryptAndPrint(const string& label, const string& base64_ct,
                     CryptoContext<DCRTPoly> cc,
                     const PrivateKey<DCRTPoly>& sk) {
    Ciphertext<DCRTPoly> ct = DeserializeCiphertextFromBase64(base64_ct);
    Plaintext pt;
    cc->Decrypt(sk, ct, &pt);
    pt->SetLength(pt->GetLength());

    cout << "Decrypted " << label << ": ";
    for (const auto& val : pt->GetCKKSPackedValue())
        cout << val.real() << " ";
    cout << "\n";
}

int main() {
    try {
        int round = getCurrentRound();
        cout << "[Client2 Receiver] Using round " << round << endl;

        string url = "http://localhost:8000/result?round=" + to_string(round) + "&client_id=client2";

        // STEP 1: Fetch results
        json result = json::parse(HttpGetJson(url));
        string sum_b64 = result["results"]["sum"];
        string product_b64 = result["results"]["product"];

        // STEP 2: Load CryptoContext and Private Key
        CryptoContext<DCRTPoly> cc;
        ifstream ccStream("cc.bin", ios::binary);
        Serial::Deserialize(cc, ccStream, SerType::BINARY);

        PrivateKey<DCRTPoly> sk;
        ifstream skStream("client2_private.key", ios::binary);
        Serial::Deserialize(sk, skStream, SerType::BINARY);

        // STEP 3: Decrypt both results
        decryptAndPrint("SUM", sum_b64, cc, sk);
        decryptAndPrint("PRODUCT", product_b64, cc, sk);

        // STEP 4: Log metadata
        cout << "Model: " << result["model"]
             << "\nVersion: " << result["version"]
             << "\nRound: " << result["round"]
             << "\nDirection: " << result["direction"] << "\n";
        cout << "-----------------------------------------------------\n";

    } catch (const std::exception& e) {
        cerr << "[Client2 Receiver] Error: " << e.what() << endl;
        cout << "-----------------------------------------------------\n";
        return 1;
    }

    return 0;
}
