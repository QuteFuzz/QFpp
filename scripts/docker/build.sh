#!/bin/bash

# config filesystem
cat > ~/.config/containers/storage.conf << 'EOF'
[storage]
driver = "overlay"
runroot = "/tmp/$USER/containers/run"
graphroot = "/tmp/$USER/containers/storage"

[storage.options.overlay]
mount_program = "/usr/bin/fuse-overlayfs"
EOF

mkdir -p /tmp/$USER/containers/{run,storage}

# create image
echo "Creating docker image"
docker build -t qutefuzz:latest .
