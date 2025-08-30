import pandas as pd
import matplotlib.pyplot as plt

# Read CSVs without headers; assign columns
c1_df = pd.read_csv('client1_data/accuracy_log.csv', header=None, names=['round', 'accuracy', 'roc_auc'])
c2_df = pd.read_csv('client2_data/accuracy_log.csv', header=None, names=['round', 'accuracy', 'roc_auc'])

plt.figure(figsize=(8, 5))
plt.plot(c1_df['round'], c1_df['accuracy'], marker='o', color='blue', label='Client 1')
plt.plot(c2_df['round'], c2_df['accuracy'], marker='o', color='green', label='Client 2')
plt.xlabel("Round")
plt.ylabel("Accuracy (MSE)")
plt.title("Accuracy per Round: Client 1 vs Client 2")
plt.xticks(c1_df['round'])
plt.legend()
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("accuracy_plot.png")
print("Saved plot to accuracy_plot.png")

# Timing plot
df = pd.read_csv("timing_rounds.csv", header=None, names=["round", "time_sec"])
plt.figure(figsize=(8,5))
plt.plot(df["round"], df["time_sec"], marker='o', color='purple')
plt.xlabel("Round")
plt.ylabel("Time per Round (seconds)")
plt.title("Time per Federated Learning Round")
plt.xticks(df["round"])
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("timing_per_round.png")
print("Saved plot to timing_per_round.png")


# Read communication log CSVs for both clients (no headers)
c1_comm = pd.read_csv('client1_data/comm_logs.csv', header=None, names=['round', 'client', 'event', 'size'])
c2_comm = pd.read_csv('client2_data/comm_logs.csv', header=None, names=['round', 'client', 'event', 'size'])

def plot_comm(comm_df, client_label, color_upload, color_download, fname):
    upload = comm_df[comm_df['event'] == 'upload']
    download = comm_df[comm_df['event'] == 'download']

    plt.figure(figsize=(8,5))
    plt.plot(upload['round'], upload['size'] / 1024, marker='o', color=color_upload, label='Upload (KB)')
    plt.plot(download['round'], download['size'] / 1024, marker='s', color=color_download, label='Download (KB)')
    plt.xlabel("Round")
    plt.ylabel("Payload Size (KB)")
    plt.title(f"Communication Overhead per Round: {client_label}")
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout()
    plt.savefig(fname)
    print(f"Saved plot to {fname}")

plot_comm(c1_comm, "Client 1", 'blue', 'green', "client1_comm_overhead.png")
plot_comm(c2_comm, "Client 2", 'red', 'orange', "client2_comm_overhead.png")

