import os
import pandas as pd
import numpy as np
import json
import sys
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import LSTM, Dense, Input
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.losses import BinaryCrossentropy
from sklearn.preprocessing import StandardScaler
import pickle

def extract_time_features(df):
    # "Time" is 'HH:MM:SS'
    time_parts = df['Time'].str.split(":", expand=True).astype(int)
    df['hour'] = time_parts[0]
    df['minute'] = time_parts[1]
    df['second'] = time_parts[2]
    df['seconds_since_midnight'] = df['hour']*3600 + df['minute']*60 + df['second']

def extract_date_features(df):
    # "Date" is 'YYYY-MM-DD'
    date_col = pd.to_datetime(df['Date'], errors='coerce')
    df['year'] = date_col.dt.year
    df['month'] = date_col.dt.month
    df['day'] = date_col.dt.day
    df['weekday'] = date_col.dt.weekday
    df['dayofyear'] = date_col.dt.dayofyear

def preprocess_data(
    df, 
    window_size=5, 
    category_columns_path=None, 
    fit=True, 
    use_accounts=False
):
    """
    Main preprocessing function for training. Saves or loads category columns order for future alignment.
    """
    extract_time_features(df)
    extract_date_features(df)
    # Numeric features
    numeric_cols = [
        'Amount', 'seconds_since_midnight', 'hour', 'minute', 'second',
        'year', 'month', 'day', 'weekday', 'dayofyear'
    ]
    # Categorical features
    categorical_cols = [
        'Payment_type', 'Payment_currency', 'Received_currency',
        'Sender_bank_location', 'Receiver_bank_location'
    ]
    if use_accounts:
        categorical_cols += ['Sender_account', 'Receiver_account']

    # One-hot encode categoricals
    cat_encoded = pd.get_dummies(df[categorical_cols].astype(str))

    if fit:
        cat_columns = list(cat_encoded.columns)
        if category_columns_path:
            with open(category_columns_path, "w") as fp:
                json.dump(cat_columns, fp)
    else:
        with open(category_columns_path, "r") as fp:
            cat_columns = json.load(fp)
        cat_encoded = cat_encoded.reindex(columns=cat_columns, fill_value=0)
    
    # Assemble full features
    features = pd.concat([df[numeric_cols], cat_encoded], axis=1)
    features = features.apply(pd.to_numeric, errors='coerce').fillna(0).astype(np.float32)

    # Normalize numeric features
    scaler = StandardScaler()
    features[numeric_cols] = scaler.fit_transform(features[numeric_cols])

    labels = df['Is_laundering'].values.astype(np.float32)

    # Build sliding window sequences
    X_seq = []
    y_seq = []
    X_values = features.values
    for i in range(len(df) - window_size):
        X_seq.append(X_values[i:i+window_size])
        y_seq.append(labels[i+window_size])
    X_seq = np.array(X_seq, dtype=np.float32)
    y_seq = np.array(y_seq, dtype=np.float32)
    return X_seq, y_seq, scaler, cat_columns

def create_lstm_model(input_shape):
    model = Sequential()
    model.add(Input(shape=input_shape))
    model.add(LSTM(50, activation='relu'))
    model.add(Dense(1, activation='sigmoid'))
    model.compile(optimizer=Adam(learning_rate=0.001), loss=BinaryCrossentropy(), metrics=['accuracy'])
    return model

def set_model_weights_from_json(model, weights_json):
    weights = [np.array(w) for w in weights_json]
    model.set_weights(weights)

def get_model_weights_as_json(model):
    weights = model.get_weights()
    return [w.tolist() for w in weights]

def train_model(
    train_csv,
    model_h5_path,
    warm_start_json=None,
    epochs=10,
    batch_size=16,
    window_size=5,
    use_accounts=False
):
    category_column_path = "client2_data/category_columns.json"
    scaler_path = "client2_data/scaler.pkl"
    weights_json_path = "client2_data/wc.json"

    # --- Data loading and preprocessing (fit structure fresh) ---
    df = pd.read_csv(train_csv)
    X_train, y_train, scaler, cat_columns = preprocess_data(
        df,
        window_size,
        category_columns_path=category_column_path,
        fit=True,
        use_accounts=use_accounts
    )

    model = create_lstm_model(input_shape=(window_size, X_train.shape[2]))

    if warm_start_json and os.path.isfile(warm_start_json) and os.path.getsize(warm_start_json) > 0:
        try:
            with open(warm_start_json, "r") as f:
                weights_json = json.load(f)
            set_model_weights_from_json(model, weights_json)
            print(f"[client2_train.py] Warm start: Loaded model weights from {warm_start_json}")
        except Exception as e:
            print(f"[client2_train.py] WARNING: Could not load warm start weights: {e}")
    else:
        print("[client2_train.py] No warm start file found or provided. Training from scratch.")

    model.fit(X_train, y_train, epochs=epochs, batch_size=batch_size, verbose=2)

    model.save(model_h5_path)
    weights_json = get_model_weights_as_json(model)
    with open(weights_json_path, "w") as f:
        json.dump(weights_json, f, indent=2)
    with open(scaler_path, "wb") as f:
        pickle.dump(scaler, f)
    print(f"[client2_train.py] Model and weights saved to {model_h5_path}, {weights_json_path}.\nScaler/column info saved for future rounds.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python client2_train.py <train_csv> <model_h5> [warm_start_json] [epochs] [batch_size] [window_size] [use_accounts]")
        sys.exit(1)
    train_csv = sys.argv[1]
    model_h5 = sys.argv[2]
    warm_start = sys.argv[3] if len(sys.argv) >= 4 and sys.argv[3] != "" else None
    epochs = int(sys.argv[4]) if len(sys.argv) >= 5 else 10
    batch_size = int(sys.argv[5]) if len(sys.argv) >= 6 else 16
    window_size = int(sys.argv[6]) if len(sys.argv) >= 7 else 5
    use_accounts = bool(int(sys.argv[7])) if len(sys.argv) >= 8 else False

    train_model(train_csv, model_h5, warm_start, epochs, batch_size, window_size, use_accounts)

