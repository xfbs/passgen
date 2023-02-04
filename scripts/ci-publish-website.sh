#!/bin/bash

mkdir docs/images
curl -L 'https://unsplash.com/photos/kgRBoAKq-4E/download?force=true&w=2400' --output docs/images/randomness.jpg
curl -L 'https://unsplash.com/photos/XRNSn4gt8Hs/download?force=true&w=2400' --output docs/images/chaos.jpg
curl -L 'https://unsplash.com/photos/t5YUoHW6zRo/download?force=true&w=2400' --output docs/images/tooling.jpg
xz -d docs/media/*.xz
mkdocs build --site-dir public
