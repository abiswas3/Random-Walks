#!/bin/bash

# Build site
hugo || { echo "Hugo build failed"; exit 1; }

# Sync to target folder
rsync -avz --delete public/ ~/abiswas3.github.io/

