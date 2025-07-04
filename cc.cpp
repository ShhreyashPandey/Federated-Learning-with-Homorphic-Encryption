#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace lbcrypto;
using namespace std;

unordered_map<string, string> LoadConfig(const string& filename) {
    unordered_map<string, string> config;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t eqPos = line.find('=');
        if (eqPos == string::npos) continue;
        string key = line.substr(0, eqPos);
        string val = line.substr(eqPos + 1);
        config[key] = val;
    }

    return config;
}

int main() {
    auto config = LoadConfig("cc_config.txt");

    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(stoi(config["multiplicativeDepth"]));
    params.SetScalingModSize(stoi(config["scalingModSize"]));
    params.SetBatchSize(stoi(config["batchSize"]));

    string preModeStr = config["preMode"];
    if (preModeStr == "INDCPA")
        params.SetPREMode(INDCPA);
    else if (preModeStr == "INDCCA")
        params.SetPREMode(INDCPA);
    else {
        cerr << "[cc.cpp] Invalid PREMode! Use INDCPA or INDCCA." << endl;
        return 1;
    }

    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKESchemeFeature::PKE);
    cc->Enable(PKESchemeFeature::LEVELEDSHE);
    cc->Enable(PKESchemeFeature::PRE);
    cc->Enable(PKESchemeFeature::KEYSWITCH);
    cc->Enable(PKESchemeFeature::ADVANCEDSHE);

    ofstream ccOut("cc.bin", ios::binary);
    Serial::Serialize(cc, ccOut, SerType::BINARY);

    cout << "[cc.cpp] CryptoContext loaded from config and saved to cc.bin ✅" << endl;
    return 0;
}
