#!/bin/bash

mkdir -p coverage_reports

# 2. Run gcovr and point the output into that new folder
uv run gcovr \
    --root external/tket \
    --html-details coverage_reports/coverage.html \
    --print-summary \
    -j 8
