import os
import pandas as pd
from sklearn.linear_model import LinearRegression
import pickle
import json
import sys

def train_model(train_csv, model_pickle_path, wc_json_path, warm_start_json=None):
    # Load training data
    df = pd.read_csv(train_csv)
    X = df[['X']].values.reshape(-1, 1)
    y = df['y'].values

    if warm_start_json and os.path.exists(warm_start_json):
        try:
            with open(warm_start_json, 'r') as f:
                warm_params = json.load(f)
            print(f"[c2_train.py] Warm start parameters loaded: w={warm_params['w']}, c={warm_params['c']}")
            
        except Exception as e:
            print(f"[c2_train.py] Warning: Could not load warm start params: {e}")
    else:
        print("[c2_train.py] No warm start file provided or file missing; training from scratch.")

    # Train LR model
    model = LinearRegression()
    model.fit(X, y)

    # Save model pickle inside client2_data folder
    with open(model_pickle_path, 'wb') as f:
        pickle.dump(model, f)

    # Extract and save model parameters to JSON
    params = {
        "w": float(model.coef_[0]),
        "c": float(model.intercept_)
    }
    with open(wc_json_path, 'w') as f:
        json.dump(params, f)

    print(f"[c2_train.py] Model trained and saved to {model_pickle_path} and params to {wc_json_path}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python client2_train.py <train_csv> <model_pickle> <wc_json> [warm_start_json]")
        sys.exit(1)

    train_csv = sys.argv[1]
    model_pickle = sys.argv[2]
    wc_json = sys.argv[3]
    warm_start = sys.argv[4] if len(sys.argv) >= 5 else None

    train_model(train_csv, model_pickle, wc_json, warm_start)
