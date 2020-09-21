#!/bin/bash

#COMMIT=`git rev-parse --verify --short=10 @{0}`
COMMIT=`git rev-parse --verify @{0}`
BRANCH=`git rev-parse --verify --abbrev-ref HEAD`
WORKDIR_STATUS="modified files:`git diff --numstat|wc -l`, staged files:`git diff --cached --numstat|wc -l`"

CTC_GIT="$BRANCH-$COMMIT"
CTC_WORKDIR_STATUS="$WORKDIR_STATUS"
CTC_BUILD_DATE=`date +%Y-%m-%d.%H:%M:%S`
CTC_BUILD_USER=`id -un`

echo "CTC: $CTC_GIT ($CTC_BUILD_USER $CTC_BUILD_DATE)"
echo "workdir status: $CTC_WORKDIR_STATUS "

cat CTC_Version.tmpl | \
	sed "s/__CTC_GIT__/$CTC_GIT/g" | \
    sed "s/__CTC_WORKDIR_STATUS__/$CTC_WORKDIR_STATUS/g" | \
	sed "s/__CTC_BUILD_DATE__/$CTC_BUILD_DATE/g" | \
	sed "s/__CTC_BUILD_USER__/$CTC_BUILD_USER/g" \
	> CTC_Version.h
