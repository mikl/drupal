#!/bin/sh
# $Id: cron-lynx.sh,v 1.1.14.1 2005/08/17 20:03:29 dries Exp $

/usr/bin/lynx -source http://yoursite.com/cron.php > /dev/null 2>&1
