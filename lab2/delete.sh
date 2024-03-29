#!/bin/bash
source ./testlib.sh

if [ "$(whoami)" != "root" ]; then
  sudo "$0" "$@"
  exit $?
fi

unmountfs
delete_dirs