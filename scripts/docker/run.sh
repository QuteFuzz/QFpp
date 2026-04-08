#!/bin/bash

IMAGE_NAME="qutefuzz:latest"

#!/bin/bash

IMAGE_NAME="qutefuzz:latest"

docker run -it --rm -v "$(pwd):/app" $IMAGE_NAME \
    bash -c "uv sync && uv run scripts/run.py $@"

