#!/bin/bash

echo "🧹 Cleaning old builds..."
make clean

echo "🔨 Building everything..."
make || { echo "❌ Build failed. Exiting."; exit 1; }

echo "♻️  Entering continuous federated computation loop..."

while true; do
    echo "-----------------------------------------------------------------------------------------"
    echo "🔄 Starting new round:"

    echo "🔧 [1] Generating model_input.txt..."
    ./model_generator || { echo "❌ model_generator failed. Exiting."; exit 1; }

    echo "⚙️  [2] Generating CryptoContext..."
    ./cc || { echo "❌ cc failed. Exiting."; exit 1; }

    echo "📤 [3] Client 1 Sender - Encrypting input..."
    ./client1_sender || { echo "❌ client1_sender failed. Exiting."; exit 1; }

    echo "📤 [4] Client 2 Sender - Encrypting input..."
    ./client2_sender || { echo "❌ client2_sender failed. Exiting."; exit 1; }

    echo "🔑 [5] Generating rekey - Client 1 ➝ 2..."
    ./client1_rekey || { echo "❌ client1_rekey failed. Exiting."; exit 1; }

    echo "🔑 [6] Generating rekey - Client 2 ➝ 1..."
    ./client2_rekey || { echo "❌ client2_rekey failed. Exiting."; exit 1; }

    echo "🖥️  [7a] Server Receiver - Performing encrypted model evaluation..."
    ./server_receiver || { echo "❌ server_receiver failed. Exiting."; exit 1; }

    echo "🖥️  [7b] Server Sender - Sharing model output..."
    ./server_sender || { echo "❌ server_sender failed. Exiting."; exit 1; }

    echo "📥 [8] Client 1 Receiver reading result..."
    ./client1_receiver || { echo "❌ client1_receiver failed. Exiting."; exit 1; }

    echo "📥 [9] Client 2 Receiver reading result..."
    ./client2_receiver || { echo "❌ client2_receiver failed. Exiting."; exit 1; }

    echo "✅ Round complete"
    sleep 1
done
