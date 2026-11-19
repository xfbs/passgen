#!/bin/bash
set -euxo pipefail

mkdir -p public

doxygen Doxyfile
mv docs/html public/docs

python3 scripts/docs_coverage.py docs/xml/ --noerror
