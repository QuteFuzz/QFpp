#!/bin/bash

echo "Creating docker image"
docker build -t qutefuzz:latest .
