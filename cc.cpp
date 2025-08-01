#include "openfhe.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "cryptocontext-ser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>

using namespace lbcrypto;
using namespace std;

// Simple config key-value loader
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

    try {
        CCParams<CryptoContextCKKSRNS> params;

        params.SetMultiplicativeDepth(stoi(config.at("multiplicativeDepth")));
        params.SetScalingModSize(stoi(config.at("scalingModSize")));
        params.SetBatchSize(stoi(config.at("batchSize")));

        if (config.count("ringDim"))
            params.SetRingDim(stoi(config.at("ringDim")));

        if (config.count("rescaleTechnique")) {
            std::string rescale = config.at("rescaleTechnique");
            if (rescale == "FIXEDMANUAL")
                params.SetScalingTechnique(lbcrypto::ScalingTechnique::FIXEDMANUAL);
            else if (rescale == "FLEXIBLEAUTO")
                params.SetScalingTechnique(lbcrypto::ScalingTechnique::FLEXIBLEAUTO);
            else if (rescale == "FLEXIBLEAUTOEXT")
                params.SetScalingTechnique(lbcrypto::ScalingTechnique::FLEXIBLEAUTOEXT);
            else if (rescale == "NORESCALE")
                params.SetScalingTechnique(lbcrypto::ScalingTechnique::NORESCALE);
            else {
                std::cerr << "[cc.cpp] Invalid rescaleTechnique in config." << std::endl;
                return 1;
            }
        }

        // PRE mode — client-defined secure param
        string preModeStr = config["preMode"];
        if (preModeStr == "INDCPA")
            params.SetPREMode(INDCPA);
        else if (preModeStr == "INDCCA") // Still INDCPA internally by OpenFHE
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
        if (!ccOut.is_open()) {
            cerr << "[cc.cpp] Failed to open cc.bin for writing!" << endl;
            return 1;
        }

        Serial::Serialize(cc, ccOut, SerType::BINARY);
        ccOut.close();

        cout << "[cc.cpp] CryptoContext created and saved to cc.bin" << endl;

    } catch (const std::exception& e) {
        cerr << "[cc.cpp] Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
