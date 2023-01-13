#!/bin/bash

mv docs/mkdocs.yml .
mkdir docs/images
curl -L 'https://unsplash.com/photos/kgRBoAKq-4E/download?force=true&w=2400' --output docs/images/randomness.jpg
curl -L 'https://unsplash.com/photos/XRNSn4gt8Hs/download?force=true&w=2400' --output docs/images/chaos.jpg
mkdocs build --site-dir public
