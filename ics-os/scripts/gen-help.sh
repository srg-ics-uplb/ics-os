#!/bin/bash
grep -rin "strcmp(u" kernel/console/console.c | sed -e 's/^[^"]*"\([^"]*\)".*\/\//\1/'
