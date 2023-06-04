#!/bin/sh

exec 2>/dev/null
find "./src/$1" -type f -iname "*.cc"
