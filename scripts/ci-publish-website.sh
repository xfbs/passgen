#!/bin/bash

xz -d docs/media/*.xz
mkdocs build --site-dir public --strict
