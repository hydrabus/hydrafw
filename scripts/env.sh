#!/usr/bin/env bash
#set -x
set -e

# This script is used to enroll toolchain with GitHub Actions for CI/CD purposes
# But this script can be used as reference to install recommended toolchain for building hydrafw.dfu locally as well

MD5SUM=8a4a74872830f80c788c944877d3ad8c
TARBALL=gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
TARDIR=gcc-arm-none-eabi-4_9-2015q3
LINK=https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/"${TARBALL}"
CURDIR="$(pwd)"

echo  "${MD5SUM} ${TARBALL}"  >  md5.txt
wget  "${LINK}"
md5sum  -c  md5.txt
rm  md5.txt

tar  xvjf  "${TARBALL}"
rm  "${TARBALL}"

# Ensure GITHUB_CI_CD is set and is an integer, default to 0 if not set
GITHUB_CI_CD="${GITHUB_CI_CD:-0}"

if [ -n "${GITHUB_CI_PR_SHA}" ] || [ 1 -eq "${GITHUB_CI_CD}" ]; then
	# This is for GitHub Actions CI/CD tooling only
	echo  "export  PATH=\"${CURDIR}/${TARDIR}/bin\":\"\${PATH}\"" > build.env
else
	# And this is for local install
	echo ""
	echo  "Add a line like"
	echo "export  PATH=\"${CURDIR}/${TARDIR}/bin\":\"\${PATH}\""
	echo "to your ~/.profile and/or ~/.bashrc and run the line to enable toolchain as default one for arm-none-eabi-* target"
	echo ""
fi;
