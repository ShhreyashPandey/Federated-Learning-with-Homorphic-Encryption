import pandas as pd
import pickle
import json
import sys
import requests
import os
import csv
from sklearn.metrics import mean_squared_error

def test_and_post_accuracy(test_csv, model_pickle_path, accuracy_json_path, accuracy_log_csv, server_url, client_id="client2"):
    # Load test data
    df = pd.read_csv(test_csv)
    X = df['X'].values
    y_true = df['y'].values

    # Load trained model from pickle
    try:
        with open(model_pickle_path, 'rb') as f:
            model = pickle.load(f)
    except Exception as e:
        print(f"[c2_test.py] Error loading model pickle: {e}")
        return

    # Predict with the model
    y_pred = model.predict(X.reshape(-1, 1))

    # Calculate mean squared error
    mse = mean_squared_error(y_true, y_pred)

    # Save current round number
    try:
        with open("round_counter.txt", "r") as f:
            server_round = f.read().strip()
    except Exception as e:
        print(f"[c2_test.py] Could not read round_counter.txt: {e}")
        server_round = "1"  # fallback

    try:
        round_int = int(server_round)
    except:
        round_int = 1

    # Save for this round in JSON
    with open(accuracy_json_path, 'w') as f:
        json.dump({"accuracy": mse, "round": round_int}, f)

    print(f"[c2_test.py] Test MSE: {mse}. Saved to {accuracy_json_path}")

    # Append to CSV for all rounds (main log for plotting)
    file_exists = os.path.isfile(accuracy_log_csv)
    with open(accuracy_log_csv, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if not file_exists:
            writer.writerow(["round", "accuracy"])
        writer.writerow([round_int, mse])

    # POST request to the server
    payload = {
        "client_id": client_id,
        "round": round_int,
        "accuracy": mse,
        "model": "LinearRegression"
    }

    # POST the accuracy JSON payload to server endpoint
    try:
        response = requests.post(server_url, json=payload)
        print(f"[c2_test.py] POST accuracy response: Status {response.status_code}, Body: {response.text}")
    except requests.RequestException as e:
        print(f"[c2_test.py] POST request failed: {e}")

if __name__ == "__main__":
    # python client2_test.py <test_csv> <model_pickle> <accuracy_json> <server_url> [<accuracy_log_csv>]
    if len(sys.argv) < 5:
        print("Usage: python client2_test.py <test_csv> <model_pickle> <accuracy_json> <server_url> [<accuracy_log_csv>]")
        sys.exit(1)

    test_csv = sys.argv[1]
    model_pickle = sys.argv[2]
    accuracy_json = sys.argv[3]
    server_url = sys.argv[4]

    # Default CSV log path is client2_data/accuracy_log.csv
    if len(sys.argv) >= 6:
        accuracy_log_csv = sys.argv[5]
    else:
        accuracy_log_csv = os.path.join(os.path.dirname(accuracy_json), "accuracy_log.csv")

    test_and_post_accuracy(test_csv, model_pickle, accuracy_json, accuracy_log_csv, server_url)

