#!/usr/bin/env bash

# Run this script to automatically fix directory structure of "old style" workspace from before the restructuring.
# "old style" workspace means when the repo only was the contents of src directory.
# Do not run this script while the port is running.


set -e 

TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $TOOLS_DIR/../..
NEW_ROOT=`pwd`

# Back up everything
cp -rf "${NEW_ROOT}" "${NEW_ROOT}_backup1"

mv src _src
mv _src/.hg .
mv _src/.hgtags .
cp -r _src/* . && rm -r _src

cp -r area data && rm -rf area
cp -r box data && rm -rf box
cp -r clans data && rm -rf clans
cp -r gods data && rm -rf gods
cp -r log data && rm -rf log
cp -r player data && rm -rf player
cp -r user data && rm -rf user
