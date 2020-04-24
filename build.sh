#!/bin/bash

BASEDIR=$(pwd)

if [ $# == 1 ]; then
	SERVICES="$1"
else
	SERVICES=$(ls src/)
fi

output() {
	echo -e "\e[35;1m>> $1\e[0m"
}

mkdir $BASEDIR/build 2>/dev/null

for S in $SERVICES; do

	if [ "${S}" == "genericd" ]; then
		continue
	fi

	output "Building ${S}"

	cd $BASEDIR/src/${S}/
	ID=$(cat ID)
	V=$(cat VERSION)
	PKG="${S}_${V}"
	make

	cd $BASEDIR/build/

	mkdir ${PKG}
	mkdir ${PKG}/usr
	mkdir ${PKG}/usr/bin
	mkdir ${PKG}/usr/share
	mkdir ${PKG}/usr/share/${S}
	mkdir ${PKG}/etc
	mkdir ${PKG}/etc/systemd
	mkdir ${PKG}/etc/systemd/system
	mkdir ${PKG}/var
	mkdir ${PKG}/var/${S}
	chown root:$ID ${PKG}/var/${S}
	chmod 770 ${PKG}/var/${S}

	cp $BASEDIR/src/${S}/${S} ${PKG}/usr/bin/
	cp $BASEDIR/src/${S}/VERSION ${PKG}/usr/share/${S}/
	cp $BASEDIR/src/${S}/CHANGELOG ${PKG}/usr/share/${S}/
	cp $BASEDIR/src/${S}/${S}.service ${PKG}/etc/systemd/system/

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
	rm -rf $BASEDIR/src/${S}/${PKG}.deb

	cp $BASEDIR/build/${PKG}.deb /var/www/html/debian/

done

cd /var/www/html/debian
dpkg-scanpackages . | gzip -c9 > Packages.gz
