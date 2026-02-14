#!/bin/bash

IMAGE_NAME="qutefuzz:latest"

echo "Building docker image"
docker build -t $IMAGE_NAME .

docker run -it --rm -v $(pwd)/nightly_results:/app/nightly_results $IMAGE_NAME \
    uv run scripts/run.py "$@"
