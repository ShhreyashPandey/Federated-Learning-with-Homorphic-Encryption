#!/bin/bash

echo 1 > round_counter.txt
MAX_ROUNDS=$(grep "rounds=" loop_config.txt | cut -d'=' -f2)
echo "Configured to run $MAX_ROUNDS round(s)."

while true; do
    CURRENT_ROUND=$(cat round_counter.txt)
    if [ "$CURRENT_ROUND" -gt "$MAX_ROUNDS" ]; then
        echo "All $MAX_ROUNDS round(s) completed."
        break
    fi

    echo "-----------------------------------------------------------------------------------------"
    echo "Starting round $CURRENT_ROUND..."

    ./model_generator || { echo "❌ model_generator failed. Exiting."; exit 1; }
    ./cc || { echo "❌ cc failed. Exiting."; exit 1; }

    ./client1_sender || { echo "❌ client1_sender failed. Exiting."; exit 1; }
    ./client2_sender || { echo "❌ client2_sender failed. Exiting."; exit 1; }

    ./client1_rekey || { echo "❌ client1_rekey failed. Exiting."; exit 1; }
    ./client2_rekey || { echo "❌ client2_rekey failed. Exiting."; exit 1; }

    ./server_receiver || { echo "❌ server_receiver failed. Exiting."; exit 1; }
    ./server_sender   || { echo "❌ server_sender failed. Exiting."; exit 1; }

    ./client1_receiver || { echo "❌ client1_receiver failed. Exiting."; exit 1; }
    ./client2_receiver || { echo "❌ client2_receiver failed. Exiting."; exit 1; }

    echo $((CURRENT_ROUND + 1)) > round_counter.txt

    sleep 1
done
