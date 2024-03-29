#!/usr/bin/env bash
set -e

set +e
set -x

git describe --tags  HEAD^1
git describe --tags

echo "-------------------------"

echo "${GITHUB_CI_PR_SHA}"
sha="`git  rev-parse  --short=8  HEAD`"
tag="`git  tag  --points-at  ${sha}`"
git  rev-parse  --symbolic-full-name  --short  HEAD
date +%Y-%m-%d
git  log  --pretty=oneline  v0.11..master
git  log  --pretty=oneline  v0.11..master
git  log  --pretty=oneline  v0.11...master
git  log  --pretty=oneline  v0.11...master

git branch -a

fw_tag="`git describe --tags --abbrev=0`"
fw_id="`git  rev-list  -n 1  ${fw_tag}`"

git  log  --pretty=oneline  ${fw_id}..origin/master
git  log  --pretty=oneline  ${fw_id}..origin/master | wc -l
NUMERB22=git  log  --pretty=oneline  ${fw_id}..origin/master | wc -l

echo "====="
echo "${NUMBER22}"
echo "====="

git  log  --pretty=oneline  d6d65865716adc1419cc89e59d05f99dc99f89a9..origin/master
git  log  --pretty=oneline  v0.11..origin/master
git  log  --pretty=oneline  d6d65865716adc1419cc89e59d05f99dc99f89a9..remotes/origin/master
git  log  --pretty=oneline  d6d65865716adc1419cc89e59d05f99dc99f89a9..hydrabus/master

echo `git describe --tags --abbrev=0`-`git  log  --pretty=oneline  v0.11..master  |  wc -l`

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

tar  xvjf  "${TARBALL}"

echo  "export  PATH=\"${CURDIR}/${TARDIR}/bin\":\"\${PATH}\"" > build.env

