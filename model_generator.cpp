#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

#include "curl_utils.h"

using json = nlohmann::json;
using namespace std;

int getCurrentRound(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Failed to open round_counter.txt");
    }
    int round;
    file >> round;
    file.close();
    if (round <= 0) {
        throw runtime_error("Invalid round number in round_counter.txt");
    }
    return round;
}

int loadMaxRounds(const string& filename) {
    ifstream file(filename);
    int rounds = 10;
    string line;
    while (getline(file, line)) {
        if (line.rfind("rounds=", 0) == 0) {
            return stoi(line.substr(7));
        }
    }
    return rounds;
}

int main() {
    string loopConfig = "loop_config.txt";
    string counterFile = "round_counter.txt";

    try {
        int maxRounds = loadMaxRounds(loopConfig);
        int round = getCurrentRound(counterFile);

        if (round > maxRounds) {
            cout << "[model_generator] All rounds complete (" << round - 1 << "/" << maxRounds << ")." << endl;
            return 0;
        }

        // Static input vector
        vector<double> input_vector = {1.2, 2 ,3 ,4 ,5 ,6};

        json inputJson = {
            {"model", "LinearRegression"},
            {"version", "1.0"},
            {"round", round},
            {"input_vector", input_vector},
            {"encrypted_weights", "none"},
            {"direction", "forward"}
        };

        string response = HttpPostJson("http://localhost:8000/input", inputJson.dump());
        cout << "[model_generator] Posted model input for round " << round << ": " << response << endl;
        cout << "-----------------------------------------------------\n";

    } catch (const std::exception& e) {
        cerr << "[model_generator] Failed: " << e.what() << endl;
        cout << "-----------------------------------------------------\n";
        return 1;
    }

    return 0;
}
