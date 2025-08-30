import kagglehub
import os
import pandas as pd
from sklearn.model_selection import train_test_split

def download_and_split():
    # Step 1: Download dataset
    print("Downloading dataset...")
    dataset_path = kagglehub.dataset_download("berkanoztas/synthetic-transaction-monitoring-dataset-aml")
    print("Dataset downloaded at:", dataset_path)

    # List files to find the main CSV file
    csv_file = None
    for root, dirs, files in os.walk(dataset_path):
        for fname in files:
            if fname.endswith(".csv"):
                csv_file = os.path.join(root, fname)
                print("Found CSV file:", csv_file)
                break
        if csv_file:
            break

    if csv_file is None:
        raise FileNotFoundError("No CSV file found in the downloaded dataset directory")

    # Step 2: Load data
    print("Loading CSV dataset...")
    df = pd.read_csv(csv_file)
    print(f"Loaded {len(df)} rows.")

    # Optional: shuffle the dataset for fair distribution
    df = df.sample(frac=1, random_state=42).reset_index(drop=True)

    # Step 3: Split dataset into two clients, equal halves
    midpoint = len(df) // 2
    client1_df = df.iloc[:midpoint].reset_index(drop=True)
    client2_df = df.iloc[midpoint:].reset_index(drop=True)
    print(f"Split data into 2 clients: {len(client1_df)} rows and {len(client2_df)} rows")

    # Step 4: Train/test split 80/20 for each client
    c1_train, c1_test = train_test_split(client1_df, test_size=0.2, random_state=1)
    c2_train, c2_test = train_test_split(client2_df, test_size=0.2, random_state=1)

    # Step 5: Save to client folders
    os.makedirs("client1_data", exist_ok=True)
    os.makedirs("client2_data", exist_ok=True)

    c1_train.to_csv("client1_data/data1.csv", index=False)
    c1_test.to_csv("client1_data/test1.csv", index=False)
    c2_train.to_csv("client2_data/data2.csv", index=False)
    c2_test.to_csv("client2_data/test2.csv", index=False)
    print("Data splits saved to 'client1_data/' and 'client2_data/'")

if __name__ == "__main__":
    download_and_split()

