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

incdir="$(pwd)"/include
libtemplate='src/libtemplate.cpp'
shared_object="$tmpdir"/libspec.so

echo 'Compiling spec...'
./yavl-compiler "$spec_file" "$tmpdir"/spec.h

echo 'Compiling shared object...'
g++ -shared -fPIC "$libtemplate" -I"$tmpdir" -I"$incdir" -o "$shared_object" 

echo 'Validating...'
./yavl-validator "$sample_file" "$shared_object" "$type_name"
