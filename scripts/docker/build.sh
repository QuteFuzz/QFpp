#!/bin/bash

# config filesystem
cat > ~/.config/containers/storage.conf << 'EOF'
[storage]
driver = "vfs"
runroot = "/tmp/containers-run-$UID"
graphroot = "$HOME/.local/share/containers/storage"
EOF

mkdir -p "/tmp/containers-run-$UID"
mkdir -p $HOME/.local/share/containers/storage

# create image
echo "Creating docker image"
docker build -t qutefuzz:latest .
