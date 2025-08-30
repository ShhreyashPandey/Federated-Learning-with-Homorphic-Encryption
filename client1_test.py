import pandas as pd
import numpy as np
import json
import sys
import requests
import os
import csv
import pickle
from tensorflow.keras.models import load_model
from sklearn.metrics import accuracy_score, roc_auc_score

def extract_time_features(df):
    time_parts = df['Time'].str.split(":", expand=True).astype(int)
    df['hour'] = time_parts[0]
    df['minute'] = time_parts[1]
    df['second'] = time_parts[2]
    df['seconds_since_midnight'] = df['hour']*3600 + df['minute']*60 + df['second']

def extract_date_features(df):
    date_col = pd.to_datetime(df['Date'], errors='coerce')
    df['year'] = date_col.dt.year
    df['month'] = date_col.dt.month
    df['day'] = date_col.dt.day
    df['weekday'] = date_col.dt.weekday
    df['dayofyear'] = date_col.dt.dayofyear

def preprocess_test_data(df, scaler, category_columns_path, window_size=5, use_accounts=False):
    # Extract time/date features as in training
    extract_time_features(df)
    extract_date_features(df)

    # Numeric columns: same as training
    numeric_cols = [
        'Amount', 'seconds_since_midnight', 'hour', 'minute', 'second',
        'year', 'month', 'day', 'weekday', 'dayofyear'
    ]

    # Categorical columns: same as training
    categorical_cols = [
        'Payment_type', 'Payment_currency', 'Received_currency',
        'Sender_bank_location', 'Receiver_bank_location'
    ]
    if use_accounts:
        categorical_cols += ['Sender_account', 'Receiver_account']

    # One-hot encode categorical columns
    cat_encoded = pd.get_dummies(df[categorical_cols].astype(str))

    # Load category columns order saved during training, ensure consistent columns and order
    with open(category_columns_path, "r") as fp:
        cat_columns = json.load(fp)
    cat_encoded = cat_encoded.reindex(columns=cat_columns, fill_value=0)
    
    # Concatenate numeric and categorical features
    features = pd.concat([df[numeric_cols], cat_encoded], axis=1)

    # Convert all data to numeric type (floats) and fill missing values if any
    features = features.apply(pd.to_numeric, errors='coerce').fillna(0).astype(np.float32)

    # Scale only numeric columns with saved scaler
    features[numeric_cols] = scaler.transform(features[numeric_cols])

    X_values = features.values

    # Create sliding window sequences of features
    X_seq = []
    for i in range(len(df) - window_size):
        X_seq.append(X_values[i:i+window_size])
    X_seq = np.array(X_seq, dtype=np.float32)

    return X_seq

def test_and_post_metrics(test_csv, model_h5_path, metrics_json_path, accuracy_log_csv, server_url, client_id="client1",
                          window_size=5, use_accounts=False):
    # Load test data
    df = pd.read_csv(test_csv)

    # Load scaler and category columns saved during training
    try:
        with open("client1_data/scaler.pkl", "rb") as f:
            scaler = pickle.load(f)
    except Exception as e:
        print(f"[c1_test.py] Error loading scaler.pkl: {e}")
        return

    category_columns_path = "client1_data/category_columns.json"
    if not os.path.exists(category_columns_path):
        print(f"[c1_test.py] category_columns.json not found at {category_columns_path}")
        return

    # Check sufficient data length
    if len(df) <= window_size:
        print(f"[c1_test.py] Not enough data for window size {window_size}")
        return

    # Preprocess test data sequences
    X_test = preprocess_test_data(df, scaler, category_columns_path, window_size, use_accounts=use_accounts)

    # Load model
    try:
        model = load_model(model_h5_path)
    except Exception as e:
        print(f"[c1_test.py] Error loading model: {e}")
        return

    # Predict probabilities
    y_pred_prob = model.predict(X_test).flatten()

    # Ground truth labels adjusted for window size offset
    y_true = df['Is_laundering'].values[window_size:]

    # Compute metrics
    y_pred_label = (y_pred_prob >= 0.5).astype(int)
    acc = accuracy_score(y_true, y_pred_label)
    auc = roc_auc_score(y_true, y_pred_prob)

    print(f"[c1_test.py] Accuracy: {acc:.4f}, ROC AUC: {auc:.4f}")

    # Get current federated round
    try:
        with open("round_counter.txt", "r") as f:
            round_str = f.read().strip()
        round_num = int(round_str)
    except Exception as e:
        print(f"[c1_test.py] Could not read round_counter.txt: {e}")
        round_num = 1

    # Save metrics JSON
    metrics = {"accuracy": acc, "auc": auc, "round": round_num}
    with open(metrics_json_path, 'w') as f:
        json.dump(metrics, f, indent=2)
    print(f"[c1_test.py] Metrics saved to {metrics_json_path}")

    # Append to CSV log file keeping history
    file_exists = os.path.isfile(accuracy_log_csv)
    with open(accuracy_log_csv, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if not file_exists:
            writer.writerow(["round", "accuracy", "auc"])
        writer.writerow([round_num, acc, auc])

    # Post metrics to server
    payload = {
        "client_id": client_id,
        "round": round_num,
        "accuracy": acc,
        "auc": auc,
        "model": "LSTM"
    }
    try:
        response = requests.post(server_url, json=payload)
        print(f"[c1_test.py] POST response: Status {response.status_code}, Body: {response.text}")
    except requests.RequestException as e:
        print(f"[c1_test.py] POST failed: {e}")

if __name__ == "__main__":
    # Usage:
    # python client1_test.py <test_csv> <model_h5> <metrics_json> <server_url> [<accuracy_log_csv>] [<window_size>] [<use_accounts>]
    if len(sys.argv) < 5:
        print("Usage: python client1_test.py <test_csv> <model_h5> <metrics_json> <server_url> [<accuracy_log_csv>] [<window_size>] [<use_accounts>]")
        sys.exit(1)

    test_csv = sys.argv[1]
    model_h5 = sys.argv[2]
    metrics_json = sys.argv[3]
    server_url = sys.argv[4]
    accuracy_log_csv = sys.argv[5] if len(sys.argv) >= 6 else os.path.join(os.path.dirname(metrics_json), "accuracy_log.csv")
    window_size = int(sys.argv[6]) if len(sys.argv) >= 7 else 5
    use_accounts = bool(int(sys.argv[7])) if len(sys.argv) >=8 else False

    test_and_post_metrics(test_csv, model_h5, metrics_json, accuracy_log_csv, server_url,
                          window_size=window_size, use_accounts=use_accounts)

