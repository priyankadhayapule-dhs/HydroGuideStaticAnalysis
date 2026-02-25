#!/bin/bash

tmpfile=$(mktemp)
objdir="$1"
shift

echo "objdir is $objdir"

tar -czf "$tmpfile" -C "$objdir" $@
base64 "$tmpfile" | cat install-tools.sh.in - > install-tools.sh
chmod +x install-tools.sh
rm "$tmpfile"