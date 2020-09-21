#!/bin/bash
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
set -e

function usage() {
  cat >&2 <<EOT
Usage: setup_dev_autotest.sh [-pavnms]

Install and configure software needed to run autotest locally.
If you're just working on tests, you do not need to run this.
Options:
  -p Desired Autotest DB password. Must be non-empty.
  -a Absolute path to autotest source tree.
  -v Show info logging from build_externals.py and compile_gwt_clients.py
  -n Non-interactive mode, doesn't ask for any user input.
     Requires -p and -a to be set.
  -m Allow remote access for database.
  -s Skip steps handled via puppet in prod.
     This is a transitional flag used to skip certain steps as they are migrated
     to puppet for use in the autotest lab. Not to be used by developers.

EOT
}


function get_y_or_n_interactive {
    local ret
    while true; do
        read -p "$2" yn
        case $yn in
            [Yy]* ) ret="y"; break;;
            [Nn]* ) ret="n"; break;;
            * ) echo "Please enter y or n.";;
        esac
    done
    eval "$1=\$ret"
}

function get_y_or_n {
  local ret=$3
  if [ "${noninteractive}" = "FALSE" ]; then
    get_y_or_n_interactive sub "$2"
    ret=$sub
  fi
  eval "$1=\$ret"
}

AUTOTEST_DIR=
PASSWD=
verbose="FALSE"
noninteractive="FALSE"
remotedb="FALSE"
skip_puppetized_steps="FALSE"
while getopts ":p:a:vnmsh" opt; do
  case ${opt} in
    a)
      AUTOTEST_DIR=$OPTARG
      ;;
    p)
      PASSWD=$OPTARG
      ;;
    v)
      verbose="TRUE"
      ;;
    n)
      noninteractive="TRUE"
      ;;
    m)
      remotedb="TRUE"
      ;;
    s)
      skip_puppetized_steps="TRUE"
      ;;
    h)
      usage
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      usage
      exit 1
      ;;
  esac
done

if [[ "${skip_puppetized_steps}" == "TRUE" ]]; then
  echo "Requested to skip certain steps. Will tell you when I skip things."
fi

if [[ $EUID -eq 0 ]]; then
  echo "Running with sudo / as root is not recommended"
  get_y_or_n verify "Continue as root? [y/N]: " "n"
  if [[ "${verify}" = 'n' ]]; then
    echo "Bailing!"
    exit 1
  fi
fi

if [ "${noninteractive}" = "TRUE" ]; then
  if [ -z "${AUTOTEST_DIR}" ]; then
    echo "-a must be specified in non-interactive mode." >&2
    exit 1
  fi
  if [ -z "${PASSWD}" ]; then
    echo "-p must be specified in non-interactive mode." >&2
    exit 1
  fi
fi


if [ -z "${PASSWD}" ]; then
  read -s -p "Autotest DB password: " PASSWD
  echo
  if [ -z "${PASSWD}" ]; then
    echo "Empty passwords not allowed." >&2
    exit 1
  fi
  read -s -p "Re-enter password: " PASSWD2
  echo
  if [ "${PASSWD}" != "${PASSWD2}" ]; then
    echo "Passwords don't match." >&2
    exit 1
  fi
fi

if [ -z "${AUTOTEST_DIR}" ]; then
  CANDIDATE=$(dirname "$(readlink -f "$0")" | egrep -o '(/[^/]+)*/files')
  read -p "Enter autotest dir [${CANDIDATE}]: " AUTOTEST_DIR
  if [ -z "${AUTOTEST_DIR}" ]; then
    AUTOTEST_DIR="${CANDIDATE}"
  fi
fi


# Sanity check AUTOTEST_DIR. If it's null, or doesn't exist on the filesystem
# then die.
if [ -z "${AUTOTEST_DIR}" ]; then
  echo "No AUTOTEST_DIR. Aborting script."
  exit 1
fi

if [ ! -d "${AUTOTEST_DIR}" ]; then
  echo "Directory $AUTOTEST_DIR does not exist. Aborting script."
  exit 1
fi


SHADOW_CONFIG_PATH="${AUTOTEST_DIR}/shadow_config.ini"
echo "Autotest supports local overrides of global configuration through a "
echo "'shadow' configuration file.  Setting one up for you now."
CLOBBER=0
if [ -f ${SHADOW_CONFIG_PATH} ]; then
  get_y_or_n clobber "Clobber existing shadow config? [Y/n]: " "n"
  if [[ "${clobber}" = 'n' ]]; then
    CLOBBER=1
    echo "Refusing to clobber existing shadow_config.ini."
  else
    echo "Clobbering existing shadow_config.ini."
  fi
fi

CROS_CHECKOUT=$(readlink -f "$AUTOTEST_DIR/../../../..")

# Create clean shadow config if we're replacing it/creating a new one.
if [ $CLOBBER -eq 0 ]; then
  cat > "${SHADOW_CONFIG_PATH}" <<EOF
[AUTOTEST_WEB]
host: localhost
password: ${PASSWD}
readonly_host: localhost
readonly_user: chromeosqa-admin
readonly_password: ${PASSWD}

[SERVER]
hostname: localhost

[SCHEDULER]
drones: localhost

[CROS]
source_tree: ${CROS_CHECKOUT}
# Edit the following line as needed.
#dev_server: http://10.10.10.10:8080
enable_ssh_tunnel_for_servo: True
enable_ssh_tunnel_for_chameleon: True
enable_ssh_connection_for_devserver: True
enable_ssh_tunnel_for_moblab: True
EOF
  echo -e "Done!\n"
fi

echo "Installing needed Ubuntu packages..."
PKG_LIST="libapache2-mod-wsgi gnuplot apache2-mpm-prefork unzip \
python-imaging libpng12-dev libfreetype6-dev \
sqlite3 python-pysqlite2 git-core pbzip2 openjdk-6-jre openjdk-6-jdk \
python-crypto  python-dev subversion build-essential python-setuptools \
python-numpy python-scipy libmysqlclient-dev"

if ! sudo apt-get install -y ${PKG_LIST}; then
  echo "Could not install packages: $?"
  exit 1
fi
echo -e "Done!\n"

AT_DIR=/usr/local/autotest
echo -n "Bind-mounting your autotest dir at ${AT_DIR}..."
sudo mkdir -p "${AT_DIR}"
sudo mount --bind "${AUTOTEST_DIR}" "${AT_DIR}"
echo -e "Done!\n"

sudo chown -R "$(whoami)" "${AT_DIR}"

EXISTING_MOUNT=$(egrep "/.+[[:space:]]${AT_DIR}" /etc/fstab || /bin/true)
if [ -n "${EXISTING_MOUNT}" ]; then
  echo "${EXISTING_MOUNT}" | awk '{print $1 " already automounting at " $2}'
  echo "We won't update /etc/fstab, but you should have a line line this:"
  echo -e "${AUTOTEST_DIR}\t${AT_DIR}\tbind defaults,bind\t0\t0"
else
  echo -n "Adding aforementioned bind-mount to /etc/fstab..."
  # Is there a better way to elevate privs and do a redirect?
  sudo su -c \
    "echo -e '${AUTOTEST_DIR}\t${AT_DIR}\tbind defaults,bind\t0\t0' \
    >> /etc/fstab"
  echo -e "Done!\n"
fi

echo -n "Reticulating splines..."

if [ "${verbose}" = "TRUE" ]; then
  "${AT_DIR}"/utils/build_externals.py
  "${AT_DIR}"/utils/compile_gwt_clients.py -a
else
  "${AT_DIR}"/utils/build_externals.py &> /dev/null
  "${AT_DIR}"/utils/compile_gwt_clients.py -a &> /dev/null
fi

echo -e "Done!\n"

echo "Start setting up Database..."
get_y_or_n clobberdb "Clobber MySQL database if it exists? [Y/n]: " "n"
opts_string="-p ${PASSWD} -a ${AT_DIR}"
if [[ "${clobberdb}" = 'y' ]]; then
  opts_string="${opts_string} -c"
fi
if [[ "${remotedb}" = 'TRUE' ]]; then
  opts_string="${opts_string} -m"
fi
"${AT_DIR}"/site_utils/setup_db.sh ${opts_string}

echo -e "Done!\n"

echo "Configuring apache to run the autotest web interface..."
if [ ! -d /etc/apache2/run ]; then
  sudo mkdir /etc/apache2/run
fi
sudo ln -sf "${AT_DIR}"/apache/apache-conf \
  /etc/apache2/sites-available/autotest-server.conf

# Disable currently active default
sudo a2dissite 000-default default || true
sudo a2ensite autotest-server.conf

sudo a2enmod rewrite
sudo a2enmod wsgi
sudo a2enmod version || true  # built-in on trusty
sudo a2enmod headers
sudo a2enmod cgid

# Setup permissions so that Apache web user can read the proper files.
chmod -R o+r "${AT_DIR}"
find "${AT_DIR}"/ -type d -print0 | xargs --null chmod o+x
chmod o+x "${AT_DIR}"/tko/*.cgi
# restart server
sudo /etc/init.d/apache2 restart

# Setup lxc and base container for server-side packaging support.
sudo apt-get install lxc -y
sudo python "${AT_DIR}"/site_utils/lxc.py -s

# Set up keys for www-data/apache user.
APACHE_USER=www-data
APACHE_HOME=/var/www
APACHE_SSH_DIR="$APACHE_HOME/.ssh"
SSH_KEYS_PATH=src/third_party/chromiumos-overlay/chromeos-base/chromeos-ssh-testkeys/files
sudo mkdir -p "$APACHE_SSH_DIR"
sudo bash <<EOF
cd "${APACHE_SSH_DIR:-/dev/null}" || exit 1
sudo cp "$CROS_CHECKOUT/$SSH_KEYS_PATH/"* .
sudo tee config >/dev/null <<EOF2
Host *
User root
IdentityFile ~/.ssh/testing_rsa
EOF2
sudo chown -R "$APACHE_USER:" .
sudo chmod -R go-rwx .
EOF
if [ $? -ne 0 ]; then
  echo "apache user SSH setup failed."
fi

echo "Browse to http://localhost to see if Autotest is working."
echo "For further necessary set up steps, see https://sites.google.com/a/chromium.org/dev/chromium-os/testing/autotest-developer-faq/setup-autotest-server?pli=1"
