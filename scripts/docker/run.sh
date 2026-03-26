#!/bin/bash

IMAGE_NAME="qutefuzz:latest"

docker run -it --rm -v $(pwd):/app $IMAGE_NAME \
    uv sync \
    uv run scripts/run.py "$@"
