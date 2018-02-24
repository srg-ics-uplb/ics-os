#!/bin/bash

HELP_FILE=base/icsos.hlp

echo "ICS-OS Commands" > $HELP_FILE 
echo "---------------" >> $HELP_FILE 
grep -rin "strcmp(u" kernel/console/console.c | sed -e 's/^[^"]*"\([^"]*\)".*\/\/-/\1/' | sort >> $HELP_FILE
