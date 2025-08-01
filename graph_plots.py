import pandas as pd
import matplotlib.pyplot as plt

c1_df = pd.read_csv('client1_data/accuracy_log.csv')
c2_df = pd.read_csv('client2_data/accuracy_log.csv')

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

