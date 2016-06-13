#!/bin/bash
if [ ! "${1}" = "skip" ] ; then
#	./build_clean.sh
	./build_kernel.sh CC='$(CROSS_COMPILE)gcc' "$@"
fi

if [ -e boot.img ] ; then
	rm Wildfire-kernel-"$(git rev-parse --abbrev-ref HEAD)"-"$(cat version)".zip 2>/dev/null
	cp boot.img kernelzip/boot.img
	tail -n $(($(cat ramdisk/default.prop | wc -l) - $(grep -n "START OVERRIDE" ramdisk/default.prop | cut -d : -f 1) + 1)) ramdisk/default.prop > kernelzip/kernel.prop
	cd kernelzip/
	7z a -mx9 Wildfire-kernel-"$(git rev-parse --abbrev-ref HEAD)"-"$(cat ../version)"-tmp.zip *
	zipalign -v 4 Wildfire-kernel-"$(git rev-parse --abbrev-ref HEAD)"-"$(cat ../version)"-tmp.zip ../Wildfire-kernel-"$(git rev-parse --abbrev-ref HEAD)"-"$(cat ../version)".zip
	rm Wildfire-kernel-"$(git rev-parse --abbrev-ref HEAD)"-"$(cat ../version)"-tmp.zip
	cd ..
	ls -al Wildfire-kernel-"$(git rev-parse --abbrev-ref HEAD)"-"$(cat version)".zip
	rm kernelzip/boot.img kernelzip/kernel.prop
fi
