#!/bin/bash
# Run fay59/x86doc's Python-2 scripts using a locally-staged Python 2.7.
#
# Ubuntu 24.04 has no Python 2, so we run an interpreter extracted (not
# installed) from the Ubuntu 18.04 python2.7 debs, plus the original
# pdfminer (20140328) on PYTHONPATH. See REPRODUCE.md for how to stage these.
#
# Configure these two paths (or export them in the environment):
#   PY27_PREFIX  - dir containing usr/bin/python2.7 (debs extracted with dpkg-deb -x)
#   PDFMINER_DIR - dir containing the pdfminer/ package (pdfminer-20140328)
set -e
PY27_PREFIX="${PY27_PREFIX:-$HOME/py27root}"
PDFMINER_DIR="${PDFMINER_DIR:-$HOME/pdfminer-20140328}"

export PYTHONHOME="$PY27_PREFIX/usr"
export LD_LIBRARY_PATH="$PY27_PREFIX/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"
export PYTHONPATH="$PDFMINER_DIR:$PYTHONPATH"
export PYTHONIOENCODING=UTF-8        # keep stray unicode prints from crashing under redirection

exec "$PY27_PREFIX/usr/bin/python2.7" "$@"
