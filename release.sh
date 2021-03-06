#/bin/sh

# update version in
# - configure.ac
# - debian/changelog
# - ChangeLog

: ${SFUSER:=x42}
: ${OSXMACHINE:=priroda.local}
: ${COWBUILDER:=opendaw.local}

if test -z "$BINARYONLY"; then
	make clean
	sh autogen.sh
	./configure --enable-contrib --enable-qtgui
	make -C doc html xjadeo.pdf
	make dist

	VERSION=$(awk '/define VERSION /{print $3;}' config.h | sed 's/"//g')
	WINVERS=$(grep " VERSION " config.h | cut -d ' ' -f3 | sed 's/"//g'| sed 's/\./_/g')

	git commit -a
	git tag "v$VERSION" || (echo -n "version tagging failed. - press Enter to continue, CTRL-C to stop."; read; )

	echo -n "git push? [Y/n] "
	read -n1 a
	echo
	if test "$a" == "n" -o "$a" == "N"; then
		exit 1
	fi

	# upload to rg42.org git
	git push origin
	git push --tags
	# upload to github git
	git push github
	git push --tags github

	#upload doc to sourceforge
	sftp $SFUSER,xjadeo@web.sourceforge.net << EOF
cd htdocs/
rm *
lcd doc/
put -P xjadeo.pdf
lcd html
put -P *
chmod 0664 *.*
EOF
	#upload source to sourceforge
	sftp $SFUSER,xjadeo@frs.sourceforge.net << EOF
cd /home/frs/project/x/xj/xjadeo/xjadeo
mkdir v${VERSION}
cd v${VERSION}
put xjadeo-${VERSION}.tar.gz
EOF

fi

## build binaries..

VERSION=$(awk '/define VERSION /{print $3;}' config.h | sed 's/"//g')
WINVERS=$(grep " VERSION " config.h | cut -d ' ' -f3 | sed 's/"//g'| sed 's/\./_/g')

if [ -z "$VERSION" ]; then 
  echo "unknown VERSION number"
  exit 1;
fi

echo -n "build and upload? [Y/n] "
read -n1 a
echo
if test "$a" == "n" -o "$a" == "N"; then
	exit 1
fi

/bin/ping -q -c1 ${OSXMACHINE} &>/dev/null \
	&& /usr/sbin/arp -n ${OSXMACHINE} &>/dev/null
ok=$?
if test "$ok" != 0; then
	echo "OSX build host can not be reached."
	exit
fi

/bin/ping -q -c1 ${COWBUILDER} &>/dev/null \
	&& /usr/sbin/arp -n ${COWBUILDER} &>/dev/null
ok=$?
if test "$ok" != 0; then
	echo "Linux cowbuild host can not be reached."
	exit
fi

echo "building win32 ..."
./x-win32.sh || exit
#clean up build system
./configure --enable-contrib --enable-qtgui

echo "building linux static ..."
rm -f /tmp/xjadeo-*.tgz
ssh $COWBUILDER ~/bin/build-xjadeo.sh

ok=$?
if test "$ok" != 0; then
	echo "remote build failed"
	exit
fi

rsync -Pa $COWBUILDER:/tmp/xjadeo-i386-linux-gnu-v${VERSION}.tgz /tmp/ || exit
rsync -Pa $COWBUILDER:/tmp/xjadeo-x86_64-linux-gnu-v${VERSION}.tgz /tmp/ || exit

echo "building osx package on $OSXMACHINE ..."
ssh $OSXMACHINE << EOF
exec /bin/bash -l
cd src/xjadeo || exit 1
git pull || exit 1
git fetch --tags || exit 1
./buildmac.sh
EOF

ok=$?
if test "$ok" != 0; then
	echo "remote build failed"
	exit
fi

rsync -Pa $OSXMACHINE:/tmp/jadeo-${VERSION}.dmg /tmp/ || exit

#upload files to sourceforge
sftp $SFUSER,xjadeo@frs.sourceforge.net << EOF
cd /home/frs/project/x/xj/xjadeo/xjadeo
mkdir v${VERSION}
cd v${VERSION}
put contrib/nsi/jadeo_installer_v${WINVERS}.exe
put /tmp/xjadeo-i386-linux-gnu-v${VERSION}.tgz
put /tmp/xjadeo-x86_64-linux-gnu-v${VERSION}.tgz
put /tmp/jadeo-${VERSION}.dmg
EOF

test -x x-releasestatic.sh && ./x-releasestatic.sh
