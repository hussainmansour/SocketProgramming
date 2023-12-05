#!/bin/bash

PORT=8080
CLIENTS=50
## Run server
#./server $PORT &

# Run clients
for ((i=1; i<=CLIENTS;i++)); do
    ./client 127.0.0.1 $PORT &
done

# Wait for all clients to finish
wait

echo "All clients finished."

