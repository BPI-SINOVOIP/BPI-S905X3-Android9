#!/bin/bash
#
# Build the CDD HTML and PDF from the source files.
# From the root directory run:
#   ./cdd_gen.sh --version xx --branch xx
#
# where version is the version number and branch is the name of the AOSP branch.
#
# To run this script, you must install the jinja2 and wkhtmltopdf  packages
# using the apt-get install command.
#

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -v|--version)
    VERSION="$2"
    shift # past argument
    shift # past value
    ;;
    -b|--branch)
    BRANCH="$2"
    shift # past argument
    shift # past value
    ;;
    --default)
    DEFAULT=YES
    shift # past argument
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

echo "VERSION = ${VERSION}"
echo "BRANCH = ${BRANCH}"

current_time=$(date "+%m.%d-%H.%M")
echo "Current Time : $current_time"

filename="android-${VERSION}-cdd-${current_time}"
echo "$filename"

if [ -z "${VERSION+x}" ] || [ -z "${BRANCH+x}" ];
then
  echo "No variables!"
  python make_cdd.py --output $filename;
else
  echo "Variables!"
  python make_cdd.py --version $VERSION --branch $BRANCH --output $filename;
fi

wkhtmltopdf -B 1in -T 1in -L .75in -R .75in page $filename.html --footer-html source/android-cdd-footer.html /tmp/$filename-body.pdf
wkhtmltopdf -s letter -B 0in -T 0in -L 0in -R 0in cover source/android-cdd-cover.html /tmp/$filename-cover.pdf
