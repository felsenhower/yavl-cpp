#!/usr/bin/env bash

set -euo pipefail

sample_file="$1"
spec_file="$2"
type_name="$3"

tmpdir="$(mktemp -d)"

function cleanup() {
    rm -rf "$tmpdir"
}

trap cleanup EXIT

incdir="$(dirname "$0")"/include
libtemplate="$(dirname "$0")"'/src/libtemplate.cpp'
shared_object="$tmpdir"/libspec.so

echo 'Compiling spec...'
"$(dirname "$0")"/yavl-compiler "$spec_file" "$tmpdir"/spec.h

echo 'Compiling shared object...'
g++ -std=c++20 -shared -fPIC "$libtemplate" -I"$tmpdir" -I"$incdir" -o "$shared_object" 

echo 'Validating...'
"$(dirname "$0")"/yavl-validator "$sample_file" "$shared_object" "$type_name"
