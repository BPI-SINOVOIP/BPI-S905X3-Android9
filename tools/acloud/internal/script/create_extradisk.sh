#!/bin/bash
#
# Script for operators to create formated images.
#   first arg is the size of a disk in GB.

set -x
set -o errexit

if [ "$#" -ne 1 ]; then
  echo "The argument should be the size of a disk in GB"
  exit 2
fi

PROJECT=android-treehugger
ZONE=us-central1-f
DISK_NAME="extradisk-instance-${1}gb"
IMAGE_NAME="extradisk-image-${1}gb"
DEV_FILE="/dev/sdc"

gcloud compute disks create "${DISK_NAME}" --zone=${ZONE} --project=${PROJECT} --size="${1}GB"
gcloud compute instances attach-disk instance-disk-creation --disk "${DISK_NAME}" --zone=${ZONE} --project=${PROJECT}

gcloud compute ssh instance-disk-creation --zone=${ZONE} --project=${PROJECT} --command "sudo mkfs.ext4 -F ${DEV_FILE}"
gcloud compute ssh instance-disk-creation --zone=${ZONE} --project=${PROJECT} --command "sudo mount -o discard,defaults ${DEV_FILE} /mnt"
gcloud compute ssh instance-disk-creation --zone=${ZONE} --project=${PROJECT} --command "ls /mnt"
gcloud compute ssh instance-disk-creation --zone=${ZONE} --project=${PROJECT} --command "sudo umount /mnt"

gcloud compute instances detach-disk instance-disk-creation --disk "${DISK_NAME}" --zone=${ZONE} --project=${PROJECT}
gcloud compute images create "${IMAGE_NAME}" --source-disk-zone=${ZONE} --source-disk "${DISK_NAME}" --project=${PROJECT}
gcloud compute disks delete "${DISK_NAME}" --zone=${ZONE} --project=${PROJECT}
echo "Done image ${IMAGE_NAME} is ready."
