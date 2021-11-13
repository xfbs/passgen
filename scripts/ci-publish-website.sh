#!/bin/bash

cp docs/mkdocs.yml .
mkdocs build --site-dir public
