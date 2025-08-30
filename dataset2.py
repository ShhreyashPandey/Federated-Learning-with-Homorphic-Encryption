import kagglehub
import os
import pandas as pd
from sklearn.model_selection import train_test_split

def download_and_split():
    # Step 1: Download dataset
    print("Downloading dataset...")
    dataset_path = kagglehub.dataset_download("berkanoztas/synthetic-transaction-monitoring-dataset-aml")
    print("Dataset downloaded at:", dataset_path)

    # Locate CSV file
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
        raise FileNotFoundError("No CSV file found in the dataset")

    # Step 2: Load and shuffle data
    print("Loading CSV dataset...")
    df = pd.read_csv(csv_file)
    print(f"Total rows in dataset: {len(df)}")

    # Sample 10,000 rows for testing (20,000 per client)
    df = df.sample(n=10000, random_state=42).reset_index(drop=True)

    # Step 3: Split into two clients (20k each)
    client1_df = df.iloc[:5000].reset_index(drop=True)
    client2_df = df.iloc[5000:].reset_index(drop=True)
    print(f"Client 1 and Client 2 assigned 5000 rows each")

    # Step 4: Train/test split 50/50 → 10k train, 10k test each
    c1_train, c1_test = train_test_split(client1_df, test_size=0.6, random_state=1)
    c2_train, c2_test = train_test_split(client2_df, test_size=0.6, random_state=1)

    # Step 5: Save to folders
    os.makedirs("client1_data", exist_ok=True)
    os.makedirs("client2_data", exist_ok=True)

    c1_train.to_csv("client1_data/data1.csv", index=False)
    c1_test.to_csv("client1_data/test1.csv", index=False)
    c2_train.to_csv("client2_data/data2.csv", index=False)
    c2_test.to_csv("client2_data/test2.csv", index=False)

    print("Saved: 3k rows each to data1.csv, test1.csv, data2.csv, test2.csv")

if __name__ == "__main__":
    download_and_split()
