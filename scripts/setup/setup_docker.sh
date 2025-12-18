#!/bin/bash

# Configuration
IMAGE_NAME="qutefuzz"
CONTAINER_NAME="qutefuzz_container"
MOUNT_DIR="/qutefuzz"

# Check if the Docker image exists
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo "Image '$IMAGE_NAME' does not exist. Building..."
    docker build -t $IMAGE_NAME .

    if [ $? -ne 0 ]; then
        echo "Error: Docker build failed!"
        exit 1
    fi

    echo "Image built successfully."
else
    echo "Image '$IMAGE_NAME' already exists."
fi

# Get the absolute path of the repository root
REPO_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "Starting container '$CONTAINER_NAME'..."
echo "Mounting $REPO_PATH -> $MOUNT_DIR"

# Run the container with the repository mounted
# --rm: Remove container after exit
# -it: Interactive terminal
# -v: Mount the repository to the container
docker run -it --rm \
    --name $CONTAINER_NAME \
    -v "$REPO_PATH:$MOUNT_DIR" \
    $IMAGE_NAME \
    /bin/bash

echo "Container exited."
