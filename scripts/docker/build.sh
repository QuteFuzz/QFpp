#!/bin/bash

# config filesystem
cat > ~/.config/containers/storage.conf << 'EOF'
[storage]
driver = "overlay"
runroot = "$HOME/.local/share/containers/run"
graphroot = $HOME/.local/share/containers/storage"

[storage.options.overlay]
mount_program = "/usr/bin/fuse-overlayfs"
EOF

mkdir -p $HOME/.local/share/containers/{run,storage}

# create image
echo "Creating docker image"
docker build -t qutefuzz:latest .
