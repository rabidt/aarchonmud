#!/usr/bin/env bash

# Run this script to automatically fix directory structure of "old style" workspace from before the restructuring.
# "old style" workspace means when the repo only was the contents of src directory.
# Do not run this script while the port is running.


set -e 

TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $TOOLS_DIR/../..
NEW_ROOT=`pwd`

# Back up everything
cp -rpf "${NEW_ROOT}" "${NEW_ROOT}_backup1"

mv src _src
mv _src/.hg .
mv _src/.hgtags .
cp -rp _src/* . && rm -r _src

cp -rp area data && rm -rf area
cp -rp box data && rm -rf box
cp -rp clans data && rm -rf clans
cp -rp gods data && rm -rf gods
cp -rp log data && rm -rf log
cp -rp player data && rm -rf player
cp -rp user data && rm -rf user
