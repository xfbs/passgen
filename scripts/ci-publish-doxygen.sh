#!/bin/bash
set -euxo pipefail

mkdir -p public/badges
mkdir -p public/doxygen

doxygen Doxyfile
mv docs/html public/doxygen

python3 scripts/docs_coverage.py docs/xml/ --badge public/badges/docs.svg --noerror
