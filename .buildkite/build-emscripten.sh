#!/bin/bash

set -euo pipefail

export JOB_NAME=linters
source .buildkite/tools/setup-bazel-linux.sh

command -v node
command -v realpath

echo "--- compilation"
PATH=$PATH:$(pwd)
export PATH
tools/scripts/update-sorbet.run.sh

rm -rf _out_
mkdir -p _out_/webasm
cp bazel-bin/emscripten/sorbet-wasm.tar _out_/webasm/sorbet-wasm.tar
