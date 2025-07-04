#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <map>
#include <sstream>

using namespace lbcrypto;
using namespace std;

void wait_for_file(const string& filename) {
    while (!filesystem::exists(filename)) {
        cout << "[Server Sender] Waiting for file: " << filename << endl;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

// Parse key=value formatted metadata file
map<string, string> parseMetadata(const string& filename) {
    map<string, string> result;
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        auto pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            result[key] = value;
        }
    }
    return result;
}

int main() {
    wait_for_file("cc.bin");
    wait_for_file("encrypted_aggregated_weights.txt");
    wait_for_file("client1_private.key");

    auto metadata = parseMetadata("encrypted_aggregated_weights.txt");

    string ctFile = metadata["encrypted_weights"];
    if (ctFile.empty()) {
        cerr << "[Server Sender] ❌ 'encrypted_weights' field missing in metadata file." << endl;
        return 1;
    }

    wait_for_file(ctFile);

    // Load CryptoContext and keys
    CryptoContext<DCRTPoly> cc;
    ifstream ccStream("cc.bin", ios::binary);
    Serial::Deserialize(cc, ccStream, SerType::BINARY);

    PrivateKey<DCRTPoly> sk;
    ifstream skStream("client1_private.key", ios::binary);
    Serial::Deserialize(sk, skStream, SerType::BINARY);

    // Load ciphertext
    Ciphertext<DCRTPoly> ctSum;
    ifstream ctIn(ctFile, ios::binary);
    Serial::Deserialize(ctSum, ctIn, SerType::BINARY);

    // Decrypt result
    Plaintext ptResult;
    cc->Decrypt(sk, ctSum, &ptResult);
    ptResult->SetLength(1);
    double output = ptResult->GetCKKSPackedValue()[0].real();

    // Append to model_input.txt for reuse
    wait_for_file("model_input.txt");
    ifstream inFile("model_input.txt");
    ofstream outFile("model_input_tmp.txt");
    string line;
    while (getline(inFile, line)) {
        outFile << line << endl;
    }
    outFile << "output=" << output << endl;
    outFile << "direction=client_to_model" << endl;
    outFile.close();
    inFile.close();

    remove("model_input.txt");
    rename("model_input_tmp.txt", "model_input.txt");

    // Log the round to logs/ directory
    string round = metadata["round"];
    string logFile = "logs/round_" + round + "_output.txt";
    filesystem::create_directory("logs");

    ofstream log(logFile);
    for (const auto& [k, v] : metadata) {
        log << k << "=" << v << endl;
    }
    log << "output=" << output << endl;
    log.close();

    cout << "[Server Sender] ✅ Output appended to model_input.txt and saved to " << logFile << endl;
    return 0;
}
