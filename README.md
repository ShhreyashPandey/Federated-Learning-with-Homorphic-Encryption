# 🔒 Privacy-Preserving Federated Learning with Homomorphic Encryption

## 📌 Project Overview
This repository contains the full implementation of our research work:  
**“Privacy-Preserving Federated Learning with Homomorphic Encryption”**,  
developed by **Ishaan Nikhil Deshpande** and **Shhreyash Pandey**, under the guidance of **Dr. Puneet Bakshi (CDAC Pune)**.

The project integrates **Federated Learning (FL)** with **Homomorphic Encryption (HE)** using the **CKKS scheme** and **Proxy Re-Encryption (PRE)**.  
It ensures that model updates remain encrypted throughout the training process, enabling secure aggregation by the server **without ever accessing raw data**.

Our experiments on a **financial fraud detection dataset** using an **LSTM model** demonstrate that encrypted federated learning achieves **comparable accuracy to plaintext FL**, while providing **robust protection against inference attacks and privacy leakage**.

VIDEO EXPLANATION - 

https://drive.google.com/file/d/1yo0denaAVPKgOS83jG-Xy9-7qJqz-zgq/view?usp=drivesdk

---

## 🎯 Objectives
- Enable collaborative model training across multiple clients without exposing raw data.  
- Use Homomorphic Encryption (CKKS) to perform computations directly on encrypted updates.  
- Employ Proxy Re-Encryption (PRE) for secure multi-client aggregation.  
- Analyze accuracy, runtime, and communication overhead trade-offs.  
- Demonstrate real-world applicability in sensitive domains (finance, healthcare, defense).  

---

## ✨ Key Features
- Client Modules: Key generation, encryption, decryption, re-key generation.  
- REST API Server (`api_server.cpp`) for secure communication.  
- Homomorphic Operations: Secure aggregation without decryption.  
- Performance Tracking: Accuracy, communication overhead, timing per round.  
- Automation with Makefile and `run.sh`.  

---

## 🏗️ System Architecture

<img width="967" height="1126" alt="System Architecture Diagram" src="https://github.com/user-attachments/assets/6c25b141-bda4-4d3e-aedb-9505269f2ab6" />

### Clients
- Train local models (`client*_train.py`, `client*_test.py`).  
- Generate keys, encrypt updates, decrypt results.  
- Communicate securely with the server.  

### Server
- Acts as a message relay.  
- Stores encrypted updates, keys, and metadata.  
- Never sees plaintext data.  

### Compute Engine
- Performs proxy re-encryption to unify ciphertext domains.  
- Aggregates encrypted updates with homomorphic addition/multiplication.  
- Sends encrypted global model back to clients.  

### Threat Model
- Semi-honest (honest-but-curious).  
- Neither server nor clients see raw data.  

---

## 📂 Project Structure
- `api_server.cpp`: REST API server  
- `cc.cpp / cc.h`: CryptoContext setup  
- `cc_registry.cpp`: Registry for context  
- `cc_config.txt`: Crypto parameters  
- `client1_*/client2_*`: Client programs (keygen, encrypt, decrypt, rekey)  
- `client*_train.py`: Local training scripts  
- `client*_test.py`: Local testing scripts  
- `dataset.py / dataset2.py`: Dataset generation  
- `graph_plots.py`: Accuracy/overhead plots  
- `operations.cpp`: Homomorphic aggregation  
- `serialization_utils.*`: Serialize/deserialize ciphertexts  
- `base64_utils.*`: Encode/decode for REST transfer  
- `curl_utils.*`: HTTP communication utils  
- `rest_storage.*`: REST storage manager  
- `Makefile`: Compilation automation  
- `run.sh`: Orchestration script  
- `loop_config.txt`: Config for max rounds  
- `round_counter.txt`: Tracks current round  
- `accuracy_plot.png`: Accuracy vs rounds  
- `client*_comm_overhead.png`: Communication plots  
- `timing_per_round.png`: Round-wise timing  

---

## 🔧 Tech Stack

### Languages
- C++17  
- Python 3.9+  

### Crypto Library
- OpenFHE (CKKS scheme)  

### Networking
- libcurl (C++)  

### ML Frameworks
- PyTorch  
- Scikit-learn  

### Visualization
- Matplotlib  
- NumPy  

### Tools
- Make  
- Bash scripting  
- VMware Ubuntu  

---

## ⚙️ Installation and Setup

### Prerequisites
- Ubuntu 20.04+ (tested on VMware)  
- g++ (C++17), Make, CMake  
- Python 3.9+ with pip  

### Python Dependencies
- numpy  
- matplotlib  
- torch  
- scikit-learn  

### Build C++
```bash
make clean && make
````

### Run Federated Learning

```bash
bash run.sh
```

### Workflow Per Round

1. Clients train locally.
2. Updates are encrypted and uploaded to the server.
3. Compute Engine applies PRE and secure aggregation.
4. Encrypted global model is returned and decrypted.
5. Repeat until convergence.

### Visualize Results

```bash
python3 graph_plots.py
```

### Generated Outputs

* `accuracy_plot.png`
* `client1_comm_overhead.png`
* `client2_comm_overhead.png`
* `timing_per_round.png`

---

## 📊 Results and Analysis

* **Accuracy**: Comparable to plaintext FL, stable across rounds.
* **Communication**: Upload sizes larger due to ciphertext chunking, but manageable.
* **Timing**: First round slower (init + key exchange), subsequent rounds stabilize.
* **Trade-offs**: Slight overhead, but strong privacy guarantees.

---

## 🌍 Applications

* **Finance**: Fraud detection across banks without sharing sensitive transactions.
* **Healthcare**: Collaborative disease prediction across hospitals.
* **Defense**: Secure intelligence analysis among agencies.

---

## 👨‍💻 Authors

* Ishaan Nikhil Deshpande – Dept. of AIML, SIT Pune
* Shhreyash Pandey – Dept. of AIML, SIT Pune
* Supervisor: Dr. Puneet Bakshi – CDAC Pune

---

## 📜 License

Distributed under the **MIT License**.
You are free to use, modify, and distribute this project with proper attribution.

---

## 📖 Reference

Research Paper:
**“Privacy-Preserving Federated Learning with Homomorphic Encryption”**

