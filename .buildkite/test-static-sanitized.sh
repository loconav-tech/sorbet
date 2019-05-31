#!/bin/bash

set -euo pipefail

export JOB_NAME=test-static-sanitized
source .buildkite/tools/setup-bazel-linux.sh


err=0
./bazel test //... --config=buildfarm-sanitized-linux || err=$?

echo "--- uploading test results"

rm -rf _tmp_
mkdir -p _tmp_/log/junit/

./bazel query 'tests(//...) except attr("tags", "manual", //...)' | while read -r line; do
    path="${line/://}"
    path="${path#//}"
    cp "bazel-testlogs/$path/test.xml" _tmp_/log/junit/"${path//\//_}-${BUILDKITE_JOB_ID}.xml"
done


annotation_dir="$(mktemp -d "junit-annotate-plugin-annotation-tmp.XXXXXXXXXX")"
annotation_path="${annotation_dir}/annotation.md"

function cleanup {
  rm -rf "${annotation_dir}"
}

trap cleanup EXIT


.buildkite/tools/annotate.rb _tmp_/log/junit > "$annotation_path"

cat "$annotation_path"

if grep -q "<details>" "$annotation_path"; then
  echo "--- :buildkite: Creating annotation"
  # shellcheck disable=SC2002
  cat "$annotation_path" | buildkite-agent annotate --context junit-static-sanitized --style error --append
fi

if [ "$err" -ne 0 ]; then
    exit "$err"
fi
