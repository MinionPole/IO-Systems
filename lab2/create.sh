#!/bin/bash
source ./testlib.sh

if [ "$(whoami)" != "root" ]; then
  sudo "$0" "$@"
  exit $?
fi

makefs
create_dirs
mountfs