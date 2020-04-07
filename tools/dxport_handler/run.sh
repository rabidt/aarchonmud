#!/usr/bin/env bash
set -e
source config
# do the pwd so full path shows up in ps...maybe there's a better way
nohup python3.6 `pwd`/src/main.py >> ../../data/log/dxport_handler_log.txt &
