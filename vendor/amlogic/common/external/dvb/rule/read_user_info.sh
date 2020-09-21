#!/bin/bash

read -p "name: " NAME
read -p "email: " EMAIL

echo "AUTHOR=\"${NAME}\""
echo "EMAIL=\"${EMAIL}\""
