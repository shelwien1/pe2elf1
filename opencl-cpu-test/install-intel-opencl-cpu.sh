#!/usr/bin/env bash
# Install the Intel CPU Runtime for OpenCL Applications on Debian/Ubuntu.
#
# Follows the apt route described in
# https://www.intel.com/content/www/us/en/developer/articles/technical/intel-cpu-runtime-for-opencl-applications-with-sycl-support.html
#
# The runtime ships in the oneAPI package "intel-oneapi-runtime-opencl"; it is
# an ICD, so it also needs a loader (ocl-icd-libopencl1) to be reachable.
# Run as root (or under sudo).

set -euo pipefail

KEYRING=/usr/share/keyrings/oneapi-archive-keyring.gpg
LIST=/etc/apt/sources.list.d/oneAPI.list

if [ "$(id -u)" -ne 0 ]; then
    echo "this script needs root; re-run with sudo" >&2
    exit 1
fi

echo "==> adding Intel oneAPI apt repository"
curl -fsSL https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | gpg --dearmor --yes -o "$KEYRING"
echo "deb [signed-by=$KEYRING] https://apt.repos.intel.com/oneapi all main" > "$LIST"

echo "==> updating package lists"
apt-get update \
    -o Dir::Etc::sourcelist="sources.list.d/oneAPI.list" \
    -o Dir::Etc::sourceparts="-" \
    -o APT::Get::List-Cleanup="0"

echo "==> installing the runtime, the ICD loader and the headers"
DEBIAN_FRONTEND=noninteractive apt-get install -y \
    intel-oneapi-runtime-opencl \
    ocl-icd-libopencl1 \
    opencl-headers \
    clinfo

echo "==> registered ICDs"
ls -l /etc/OpenCL/vendors/

echo "==> devices"
clinfo --list
