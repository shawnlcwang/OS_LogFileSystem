#!/bin/bash 
# 
# File:   LLFS.sh
# Author: Shawn
# LLFS bash script is used to setup & execute LLFS and vdisk


find -type f \( -name 'GNUmakefile' -o -name 'makefile' -o -name 'Makefile' \) \
-exec bash -c 'cd "$(dirname "{LLFS}")" && make' \;

!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$(cd"$(dirname "${LLFS}" )" >/dev/null 2>&1 && pwd )
find -type f \( -name 'GNUmakefile' -o -name 'makefile' -o -name 'Makefile' \) \
-exec bash -c 'cd "$(dirname "{test01}")" && make' \;

greeting="Welcome to LLFS!"
user="$(whoami)"
DIR=$(dirname "$0")


Makefile1="${HOME}/apps/test01/Makefile"


function user_details(){
    echo "$greeting"
    echo "User Name: $user"
    echo "Bash shell version: $BASH_VERSION"
    echo "Time: $(date +%Y-%m-%d_%H%M%S)"
	echo "${Makefile1}"

}

function build_exec(){
    for f in Makefile1 
    do 
        make -C "${f}" || exit
        echo "LLFS bash script executing $f"
    done 
}

user_details

build_exec 

	echo "${Makefile2}"

export PATH= 

Makefile2=${DIR}/Makefile


DIR1="$(cd"$(dirname "${test01}" )" >/dev/null 2>&1 && pwd )"


SOURCE="${apps}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done

function find_dir(){
	for FILE in `ls -l`
	do
    if test -d $FILE
    then
      echo "$FILE is a subdirectory..."
    fi
done
}

find_dir





user_details









