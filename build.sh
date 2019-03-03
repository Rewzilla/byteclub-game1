#!/bin/bash

SERVICES=$(ls /usr/local/src/)

output() {
	echo -e "\e[35;1m>> $1\e[0m"
}

for S in $SERVICES; do

	output "Building ${S}"

	cd /usr/local/src/${S}/
	V=$(cat VERSION)
	PKG="${S}_${V}"
	make

	cd /tmp/

	mkdir ${PKG}
	mkdir ${PKG}/usr
	mkdir ${PKG}/usr/local
	mkdir ${PKG}/usr/local/bin
	mkdir ${PKG}/etc
	mkdir ${PKG}/etc/systemd
	mkdir ${PKG}/etc/systemd/system

	cp /usr/local/src/${S}/${S} ${PKG}/usr/local/bin/
	cp /usr/local/src/${S}/${S}.service ${PKG}/etc/systemd/system/

	mkdir ${PKG}/DEBIAN
	(
	cat <<-EOF
		Package: ${S}
		Version: ${V}
		Section: base
		Priority: optional
		Architecture: amd64
		Maintainer: Andrew Kramer
		Description: ${S} version ${V}
	EOF
	) > ${PKG}/DEBIAN/control

	dpkg-deb --build ${PKG}

	rm -rf ${PKG}

done
