#!/bin/bash
if [ "$(whoami)" != "root" ]; then
  sudo "$0" "$@"
  exit $?
fi

function makefs(){
  echo "Creating filesystems..."
  mkfs.vfat /dev/mydisk1 && mkfs.vfat /dev/mydisk5 && mkfs.vfat /dev/mydisk6 && mkfs.vfat /dev/mydisk7
}

function create_dirs(){
  echo "Creating directories..."
  mkdir -m 777 /mnt/disk1 && mkdir -m 777 /mnt/disk5 && mkdir -m 777 /mnt/disk6 && mkdir -m 777 /mnt/disk7
}

function mountfs(){
  echo "Mounting filesystems..."
  mount /dev/mydisk1 /mnt/disk1 && mount /dev/mydisk5 /mnt/disk5 && mount /dev/mydisk6 /mnt/disk6 && mount /dev/mydisk7 /mnt/disk7
}

function unmountfs(){
  echo "Unmounting filesystems..."
  umount /dev/mydisk1 && umount /dev/mydisk5 && umount /dev/mydisk6 && umount /dev/mydisk7
}


function delete_dirs() {
  echo "Deleting directories..."
  rm -rf /mnt/disk1 && rm -rf /mnt/disk5 && rm -rf /mnt/disk6 && rm -rf /mnt/disk7
}

function create_files() {
  dd if=/dev/urandom of=/mnt/disk1/file bs=1M count=5
  dd if=/dev/urandom of=/mnt/disk5/file bs=1M count=5
  dd if=/dev/urandom of=/mnt/disk6/file bs=1M count=5
  dd if=/dev/urandom of=/mnt/disk7/file bs=1M count=5
}

function delete_files() {
  rm /mnt/disk1/file
  rm /mnt/disk5/file
  rm /mnt/disk6/file
  rm /mnt/disk7/file
}

function cp_virtual2virtual() {
  create_files
  echo "Copying files from virtual to virtual..."
  pv /mnt/disk1/file > /mnt/disk5/file
  pv /mnt/disk5/file > /mnt/disk6/file
  pv /mnt/disk6/file > /mnt/disk7/file
  pv /mnt/disk7/file > /mnt/disk1/file
  delete_files
}

function cp_virt2realANDreal2virt() {
  create_files
  mkdir -m 777 /media/share/IO-Systems/lab2/test
  echo "Copying files from virtual to real..."
  pv /mnt/disk1/file > /media/share/IO-Systems/lab2/test/disk1
  pv /mnt/disk5/file > /media/share/IO-Systems/lab2/test/disk5
  pv /mnt/disk6/file > /media/share/IO-Systems/lab2/test/disk6
  pv /mnt/disk7/file > /media/share/IO-Systems/lab2/test/disk7
  delete_files

  create_files
  echo "Copying files from real to virtual..."
  pv /media/share/IO-Systems/lab2/test/disk1 > /mnt/disk1/file
  pv /media/share/IO-Systems/lab2/test/disk5 > /mnt/disk5/file
  pv /media/share/IO-Systems/lab2/test/disk6 > /mnt/disk6/file
  pv /media/share/IO-Systems/lab2/test/disk7 > /mnt/disk7/file
  delete_files

  rm -rf /media/share/IO-Systems/lab2/test
}



