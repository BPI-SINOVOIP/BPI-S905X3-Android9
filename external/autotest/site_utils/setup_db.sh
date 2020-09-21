#!/bin/bash
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
set -e

USAGE='Usage: setup_db.sh [-pacm]'
HELP="${USAGE}\n\n\
Setup database needed to run autotest.\n\
Options:\n\
  -p Desired Autotest DB password. Must be non-empty.\n\
  -a Path to user local autotest directiry. Default is /usr/local/autotest.\n\
  -c Clobber existing database if it exists.\n\
  -m Allow remote access for database."

PASSWD=
AT_DIR=/usr/local/autotest
clobberdb="FALSE"
remotedb="FALSE"
while getopts ":p:a:cmh" opt; do
  case ${opt} in
    p)
      PASSWD=$OPTARG
      ;;
    a)
      AT_DIR=$OPTARG
      ;;
    c)
      clobberdb="TRUE"
      ;;
    m)
      remotedb="TRUE"
      ;;
    h)
      echo -e "${HELP}" >&2
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      echo -e "${HELP}" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      echo -e "${HELP}" >&2
      exit 1
      ;;
  esac
done

if [ -z "${PASSWD}" ]; then
  echo "Option -p is required." >&2
  echo -e "${HELP}" >&2
  exit 1
fi

echo "Installing needed Ubuntu packages for mysql db."
DB_PKG_LIST="mysql-server mysql-common python-mysqldb"

if ! sudo apt-get install -y ${DB_PKG_LIST}; then
  echo "Could not install packages: $?"
  exit 1
fi
echo -e "Done!\n"

# Check if database exists, clobber existing database with user consent.
#
# Arguments: Name of the database
check_database()
{
  local db_name=$1
  echo "Setting up Database: $db_name in MySQL..."
  if mysql -u root -e ';' 2> /dev/null ; then
    PASSWD_STRING=
  elif mysql -u root -p"${PASSWD}" -e ';' 2> /dev/null ; then
    PASSWD_STRING="-p${PASSWD}"
  else
    PASSWD_STRING="-p"
  fi

  if [ -z "${PASSWD_STRING}" ]; then
    mysqladmin -u root ping >/dev/null
  else
    mysqladmin -u root "${PASSWD_STRING}" ping >/dev/null
  fi

  if [ $? -ne 0 ]; then
    sudo service mysql start
  fi

  local existing_database=$(mysql -u root "${PASSWD_STRING}" -e "SELECT \
  SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = '$db_name'")

  local sql_priv="GRANT ALL PRIVILEGES ON $db_name.* TO \
  'chromeosqa-admin'@'localhost' IDENTIFIED BY '${PASSWD}';"

  if [ "${remotedb}" = "TRUE" ]; then
    sql_priv="${sql_priv} GRANT ALL PRIVILEGES ON $db_name.* TO \
    'chromeosqa-admin'@'%' IDENTIFIED BY '${PASSWD}';"
  fi

  local sql_command="drop database if exists $db_name; \
  create database $db_name; \
  ${sql_priv} FLUSH PRIVILEGES;"

  if [[ -z "${existing_database}" || "${clobberdb}" = 'TRUE' ]]; then
    mysql -u root "${PASSWD_STRING}" -e "${sql_command}"
  else
    echo "Use existing database. Use option -c to clobber it."
  fi
  echo -e "Done!\n"
}

check_database 'chromeos_autotest_db'
check_database 'chromeos_lab_servers'

echo "Populating autotest mysql DB..."
"${AT_DIR}"/database/migrate.py sync -f
"${AT_DIR}"/frontend/manage.py syncdb --noinput
# You may have to run this twice.
"${AT_DIR}"/frontend/manage.py syncdb --noinput
"${AT_DIR}"/utils/test_importer.py
echo -e "Done!\n"

echo "Initializing chromeos_lab_servers mysql DB..."
"${AT_DIR}"/database/migrate.py sync -f -d AUTOTEST_SERVER_DB
echo -e "Done!\n"

