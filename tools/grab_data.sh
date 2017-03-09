#!/usr/bin/env bash

# Grab the latest data files from main port
# TODO: add options to grab specific directories only

set -e 

TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $TOOLS_DIR/..

rsync -rtvP m256ada@aarchonmud.com:/home/m256ada/aeaea/data .
