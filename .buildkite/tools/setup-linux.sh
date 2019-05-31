#!/usr/bin/env bash

echo "--- installing dependencies"
apt-get update
apt-get install -yy pkg-config zip g++ zlib1g-dev unzip python ruby autoconf git

if [[ -n "${CLEAN_BUILD-}" ]]; then
  echo "--- cleanup"
  rm -rf /usr/local/var/bazelcache/*
fi
