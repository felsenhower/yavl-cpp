#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

test_names="$(for f in examples/*.yaml ; do echo "$f" ; done | sed -E 's/^examples\///g;s/_spec.yaml$//g;s/_sample_correct.yaml//g;s/_sample_incorrect.yaml//g' | sort -u)"

set +e

num_tests="$(echo "$test_names" | wc -l)"

i=1
for test_name in $test_names ; do
  printf '[ Testing %d / %d: %s ]\n' "$i" "$num_tests" "$test_name"
  output="$(./validate.sh 'examples/'"$test_name"'_sample_correct.yaml' 'examples/'"$test_name"'_spec.yaml' TopType 2>&1)" || {
    echo '[ Expected successful validation, but received error: ]'
    echo "$output"
    exit 1
  }
  output="$(./validate.sh 'examples/'"$test_name"'_sample_incorrect.yaml' 'examples/'"$test_name"'_spec.yaml' TopType 2>&1)" && {
    echo '[ Expected validation error, but validation succeeded: ]'
    echo "$output"
    exit 1
  }
  i=$(( i + 1))
done

echo '[ All tests finished successfully! ]'
