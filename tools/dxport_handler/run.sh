#!/usr/bin/env bash
source config
# do the pwd so full path shows up in ps...maybe there's a better way
nohup python2.7 `pwd`/src/main.py &
