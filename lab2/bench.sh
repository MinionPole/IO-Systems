source ./testlib.sh

if [ "$(whoami)" != "root" ]; then
  sudo "$0" "$@"
  exit $?
fi

makefs
create_dirs
mountfs
cp_virtual2virtual
cp_virt2realANDreal2virt
unmountfs
delete_dirs