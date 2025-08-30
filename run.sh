#!/bin/bash

set -e
set -o pipefail

MAX_ROUNDS=$(grep -E "^rounds=" loop_config.txt | head -n1 | cut -d'=' -f2 | tr -d '[:space:]')
echo "Configured to run $MAX_ROUNDS round(s)."

# Clear .csv at the start of each run
> timing_rounds.csv
> client1_data/accuracy_log.csv
> client2_data/accuracy_log.csv
> client1_data/comm_logs.csv
> client2_data/comm_logs.csv

# Initial Setup (Run once before federated rounds)
echo "Initial setup: generating CryptoContext..."
./cc

echo "🔑 Generating client1 keys..."
./client1_keygen

echo "🔑 Generating client2 keys..."
./client2_keygen

echo "🔐 Generating client1 re-encryption key..."
./client1_rekeygen

echo "🔐 Generating client2 re-encryption key..."
./client2_rekeygen

echo "✅ Initial setup done."

# Initialize round counter
echo 1 > round_counter.txt

# Activate Python environment once
source venv/bin/activate

# Set window size to match train/test scripts
WIN_SIZE=7

while true; do
    CURRENT_ROUND=$(cat round_counter.txt)
    if [ "$CURRENT_ROUND" -gt "$MAX_ROUNDS" ]; then
        echo "✅ All $MAX_ROUNDS round(s) completed."
        break
    fi

    echo "-----------------------------------------------------------------------------------"
    echo "Starting round $CURRENT_ROUND..."
    ROUND_START=$(date +%s)

    # ---- CLIENT 1 ----
    echo "🟦 [Client 1] Training round $CURRENT_ROUND..."
    if [ "$CURRENT_ROUND" -eq "1" ]; then
        python3 client1_train.py client1_data/data1.csv client1_data/model.h5 "" 10 16 $WIN_SIZE
    else
        python3 client1_train.py client1_data/data1.csv client1_data/model.h5 client1_data/agg_wc.json 10 16 $WIN_SIZE
    fi

    echo "🟦 [Client 1] Encrypting params..."
    ./client1_encrypt

    # ---- CLIENT 2 ----
    echo "🟧 [Client 2] Training round $CURRENT_ROUND..."
    if [ "$CURRENT_ROUND" -eq "1" ]; then
        python3 client2_train.py client2_data/data2.csv client2_data/model.h5 "" 10 16 $WIN_SIZE
    else
        python3 client2_train.py client2_data/data2.csv client2_data/model.h5 client2_data/agg_wc.json 10 16 $WIN_SIZE
    fi

    echo "🟧 [Client 2] Encrypting params..."
    ./client2_encrypt

    # ---- SERVER AGGREGATION ----
    echo "🟩 [SERVER] Performing homomorphic aggregation..."
    ./operations

    # ---- CLIENT 1: DECRYPT AGGREGATED PARAMS ----
    echo "🟦 [Client 1] Decrypting aggregated params..."
    ./client1_decrypt

    # ---- CLIENT 2: DECRYPT AGGREGATED PARAMS ----
    echo "🟧 [Client 2] Decrypting aggregated params..."
    ./client2_decrypt

    # ---- CLIENT 1: TEST & POST ACCURACY ----
    echo "🟦 [Client 1] Testing and posting accuracy..."
    python3 client1_test.py client1_data/test1.csv client1_data/model.h5 client1_data/accuracy.json http://localhost:8000/c2s/result client1_data/accuracy_log.csv $WIN_SIZE

    # ---- CLIENT 2: TEST & POST ACCURACY ----
    echo "🟧 [Client 2] Testing and posting accuracy..."
    python3 client2_test.py client2_data/test2.csv client2_data/model.h5 client2_data/accuracy.json http://localhost:8000/c2s/result client2_data/accuracy_log.csv $WIN_SIZE

    ROUND_END=$(date +%s)
    ROUND_TIME=$((ROUND_END - ROUND_START))
    echo "$CURRENT_ROUND,$ROUND_TIME" >> timing_rounds.csv

    # ---- increment round ----
    echo $((CURRENT_ROUND + 1)) > round_counter.txt
    sleep 1
done

deactivate

echo "🎉 Federated rounds complete. Check logs and server for results."

