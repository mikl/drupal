#!/bin/sh
# $Id: code-clean.sh,v 1.7.10.1 2005/08/17 20:03:29 dries Exp $

find . -name "*~" -type f | xargs rm -f
find . -name ".#*" -type f | xargs rm -f
find . -name "*.rej" -type f | xargs rm -f
find . -name "*.orig" -type f | xargs rm -f
find . -name "DEADJOE" -type f | xargs rm -f
find . -type f | grep -v ".psp" | grep -v ".gif" | grep -v ".jpg" | grep -v ".png" | grep -v ".tgz" | grep -v ".ico" | grep -v "druplicon" | xargs perl -wi -pe 's/\s+$/\n/'
find . -type f | grep -v ".psp" | grep -v ".gif" | grep -v ".jpg" | grep -v ".png" | grep -v ".tgz" | grep -v ".ico" | grep -v "druplicon" | xargs perl -wi -pe 's/\t/  /g'
