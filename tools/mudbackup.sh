#!/usr/bin/env bash

# based on concepts from http://www.mikerubel.org/computers/rsync_snapshots/


TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


function backup()
{
    backup_dir=/home/m256ada/backup
    data_dir=/home/m256ada/aeaea/data

    DOW=$(date +%-u) # day of week
    DOM=$(date +%-d) # day of month

    daily=$backup_dir/snapshots/daily
    weekly=$backup_dir/snapshots/weekly
    monthly=$backup_dir/snapshots/monthly

    mkdir -p $daily
    mkdir -p $weekly
    mkdir -p $monthly

    # handle dailies
    rm -rf $daily/backup.14
    for i in `seq 13 -1 1`;
    do
        mv $daily/backup.$i $daily/backup.`expr $i + 1`
    done
    cp -al $daily/backup.0 $daily/backup.1

    mkdir -p $daily/backup.0
    rsync -aAv --delete $data_dir $daily/backup.0/

    touch $daily/backup.0 # to have an accurate timestamp of backup time

    # handle weeklies
    if [ $DOW -eq 1 ]
    then
        rm -rf $weekly/backup.4
        for i in `seq 3 -1 0`;
        do
            mv $weekly/backup.$i $weekly/backup.`expr $i + 1`
        done
        cp -al $daily/backup.0 $weekly/backup.0
    fi

    # handle monthlies
    if [ $DOM -eq 1 ]
    then
        rm -rf $monthly/backup.12
        for i in `seq 11 -1 0`;
        do
            mv $monthly/backup.$i $monthly/backup.`expr $i + 1`
        done
        cp -al $daily/backup.0 $monthly/backup.0
    fi

    # now sync to remote sites
    rsync -aAvzH --delete $backup_dir/snapshots m256ada@rooflez.com:
    rsync -aAvzH --delete $backup_dir/snapshots m256ada@chia.rooflez.com:
}

backup > $TOOLS_DIR/backuplogfile.log 2>&1
