#!/bin/bash

IMAGE_NAME="qutefuzz:latest"

docker run -it --rm \
    -v "$(pwd):/app" \
    --tmpfs /app/.venv \
    $IMAGE_NAME
