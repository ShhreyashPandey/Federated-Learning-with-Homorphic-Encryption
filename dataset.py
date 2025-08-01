import numpy as np
import pandas as pd
import os

# Function to generate linear regression dataset
def generate_dataset(n_samples=100, noise=1.0, coef=2.0, intercept=5.0, random_state=None):
    rng = np.random.default_rng(random_state)
    X = rng.uniform(0, 10, size=n_samples)
    y = intercept + coef * X + rng.normal(0, noise, size=n_samples)
    return pd.DataFrame({"X": X, "y": y})

def ensure_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

client1_dir = "client1_data"
client2_dir = "client2_data"
ensure_dir(client1_dir)
ensure_dir(client2_dir)

# Dataset for Client 1
train1 = generate_dataset(n_samples=100, noise=1.0, coef=2.0, intercept=5.0, random_state=1)
test1 = generate_dataset(n_samples=30, noise=1.0, coef=2.0, intercept=5.0, random_state=2)

train1.to_csv(f"{client1_dir}/data1.csv", index=False)
test1.to_csv(f"{client1_dir}/test1.csv", index=False)

# Dataset for Client 2 (negative slope, higher noise)
train2 = generate_dataset(n_samples=100, noise=2.0, coef=-3.0, intercept=10.0, random_state=3)
test2 = generate_dataset(n_samples=30, noise=2.0, coef=-3.0, intercept=10.0, random_state=4)

train2.to_csv(f"{client2_dir}/data2.csv", index=False)
test2.to_csv(f"{client2_dir}/test2.csv", index=False)

print("Datasets generated and saved successfully.")
