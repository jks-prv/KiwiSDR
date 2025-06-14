#!/bin/bash -e
#
# Copyright (c) 2025 John Seamons, ZL4VO/KF6VO

DEBUG=
#DEBUG=TRUE
D11=KiwiSDR_BBG_BBB_Debian_11
SHA=${D11}.sha
IMG=${D11}.img.xz
FILE="/root/${IMG}"

red="printf \033[97m\033[101m"
green="printf \033[102m"
yellow="printf \033[103m"
cyan="printf \033[106m"
norm="printf \033[m"

if [ "x${DEBUG}" = "xTRUE" ] ; then
    $red; echo "DEBUG"; $norm
fi
$yellow; echo "takes about 20 minutes depending on the speed of your Internet and SD card"; $norm
echo " "

error_exit () {
    if [ "x${DEBUG}" != "xTRUE" ] ; then
        rm -f ${SHA} ${IMG}*
    fi
	exit ${err}
}

write_failure () {
	$red; echo "SD card write error"; $norm
    err=94
    error_exit
}

flush_cache () {
	sync
	blockdev --flushbufs ${destination} || true
}

root_drive="$(cat /proc/cmdline | sed 's/ /\n/g' | grep root=UUID= | awk -F 'root=' '{print $2}' || true)"
if [ ! "x${root_drive}" = "x" ] ; then
	root_drive="$(/sbin/findfs ${root_drive} || true)"
else
	root_drive="$(cat /proc/cmdline | sed 's/ /\n/g' | grep root= | awk -F 'root=' '{print $2}' || true)"
fi
boot_drive="${root_drive%?}1"

if [ "x${boot_drive}" = "x/dev/mmcblk0p1" ] ; then
	source="/dev/mmcblk0"
	destination="/dev/mmcblk1"
	root="/dev/mmcblk1p1"
fi

if [ "x${boot_drive}" = "x/dev/mmcblk1p1" ] ; then
	source="/dev/mmcblk1"
	destination="/dev/mmcblk0"
	root="/dev/mmcblk0p1"
fi

if [ ! -b "${destination}" ] ; then
	$red; echo "no SD card detected"; $norm
    err=1
    error_exit
fi

cd /root
if [ "x${DEBUG}" = "xTRUE" ] ; then
    rm -f ${SHA}
else
    rm -f ${SHA} ${IMG}*
fi

FREE=$((`df . | tail -1 | /usr/bin/tr -s ' ' | cut -d' ' -f 4`/1000))
echo "disk free ${FREE} MB"
NEED=800
if [ "x${DEBUG}" != "xTRUE" ] ; then
    if [ ${FREE} -lt ${NEED} ] ; then
        $red; echo "not enough free disk space! (need ${NEED} MB)"; $norm
        err=92
        error_exit
    fi
fi

echo " "
echo "getting checksum file \"${SHA}\" from kiwisdr.com"
curl -Ls kiwisdr.com/files/${SHA} --output /root/${SHA}
SHA_CHECK=`cat /root/${SHA}`
echo "checksum: ${SHA_CHECK}"

echo " "
if [ ! -f ${FILE} ] ; then
    echo "getting image file \"${IMG}\" from kiwisdr.com"
    curl -Ls kiwisdr.com/files/${IMG} --output ${FILE}
else
    echo "already seem to have file \"${IMG}\""
fi

echo "computing checksum of \"${IMG}\""
SHASUM=`sha256sum ${FILE}`
set -- ${SHASUM}
SHASUM=$1
echo "checksum: ${SHASUM}"
#echo `echo ${SHASUM} | sum` `echo ${SHA_CHECK} | sum`
if [ "x${SHASUM}" != "x${SHA_CHECK}" ] ; then
	$red; echo "checksums differ!"; $norm
	err=93
    error_exit
fi
$cyan; echo "checksums match"; $norm

if [ "x${DEBUG}" = "xTRUE-OFF" ] ; then
    err=222
    error_exit
fi

echo " "
echo "copying image to SD card ${destination}"
xzcat ${FILE} | dd of=${destination} || write_failure
flush_cache
echo "reloading partition table of ${destination}"
sfdisk -R ${destination} || true
sfdisk -V ${destination} || true
sleep 2

echo " "
echo "list of block devices:"
lsblk

echo " "
echo "copying configuration data from kiwi.config to SD card"
#echo "mkdir -p /media/sd"
mkdir -p /media/sd
#echo "mount ${root} /media/sd"
mount ${root} /media/sd
#echo "rsync -av /root/kiwi.config/ /media/sd/root/kiwi.config/"
rsync -av /root/kiwi.config/ /media/sd/root/kiwi.config/ || write_failure
#echo "umount /media/sd"
umount /media/sd
#echo "flush_cache"
flush_cache

if [ "x${DEBUG}" = "xTRUE" ] ; then
    $red; echo "CAUTION: not removing file ${IMG}"; $norm
    rm -f ${SHA}
else
    rm -f ${SHA} ${IMG}*
fi

echo " "
$green; echo "SD card copy complete"; $norm
