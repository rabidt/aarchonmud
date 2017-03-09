#!/usr/bin/env bash
set -e


TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


function backup()
{
    echo "-----MUD Backup-----" 

    echo `date`

    dd=`date '+%d'`
    mm=`date '+%m'`
    Y=`date '+%Y'`
    backdate="${mm}_${dd}_${Y}"

    backup_dir="${HOME}/backup"
    tarball="${backup_dir}/${backdate}.aarchon.tar.gz"

    # save core dumps until manually deleted, but do not back them up
    if ls $HOME/aeaea/data/area/core.tmp.*
    then
        cp -rpf $HOME/aeaea/data/area/core.tmp.* $HOME/aeaea/coredumps
        rm $HOME/aeaea/data/area/core.tmp.*
    fi
    tar -czvf $HOME/backup/${backdate}.aarchon.tar.gz -C $HOME/aeaea data
}

backup > $TOOLS_DIR/backuplogfile.log 2>&1
