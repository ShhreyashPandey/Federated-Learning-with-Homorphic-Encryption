#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

int loadRoundNumber(const string& filename) {
    ifstream file(filename);
    int round = 1; // default
    if (file.is_open()) {
        file >> round;
        file.close();
    }
    return round;
}

void saveNextRoundNumber(const string& filename, int currentRound) {
    ofstream file(filename);
    if (file.is_open()) {
        file << (currentRound + 1);
        file.close();
    }
}

string loadPreviousEncryptedWeights(const string& filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.rfind("encrypted_weights=", 0) == 0) {
            return line.substr(18); // Remove "encrypted_weights="
        }
    }
    return "none"; // Default placeholder
}

int main() {
    string model = "LinearRegression";
    string version = "1.0";
    int round = loadRoundNumber("round_counter.txt");
    vector<double> input_vector = {1.2, 3.4, 5.6};

    // 🔄 Load previous encrypted_weights field if available
    string encrypted_weights = loadPreviousEncryptedWeights("model_input.txt");

    ofstream out("model_input.txt");
    if (!out.is_open()) {
        cerr << "[model_generator] Failed to open model_input.txt for writing ❌" << endl;
        return 1;
    }

    out << "model=" << model << endl;
    out << "version=" << version << endl;
    out << "round=" << round << endl;

    out << "input_vector=";
    for (size_t i = 0; i < input_vector.size(); ++i) {
        out << input_vector[i];
        if (i != input_vector.size() - 1)
            out << ",";
    }
    out << endl;

    // ✅ Add direction and encrypted_weights fields
    out << "direction=forward" << endl;
    out << "encrypted_weights=" << encrypted_weights << endl;

    out.close();
    saveNextRoundNumber("round_counter.txt", round);

    cout << "[model_generator] model_input.txt generated with round " << round << " ✅" << endl;
    return 0;
}
