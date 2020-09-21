#!/bin/bash
# Updates the set of model with the most recent ones.

set -e

BASE_URL=https://www.gstatic.com/android/text_classifier/p/live

cd "$(dirname "$0")"

for f in $(wget -O- "$BASE_URL/FILELIST"); do
  wget "$BASE_URL/$f" -O "$f"
done
