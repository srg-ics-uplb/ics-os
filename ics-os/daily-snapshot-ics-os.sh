#!/bin/bash

DTE=`date +%m%d%y`
svn export https://ics-os.googlecode.com/svn/trunk/ics-os ics-os-$DTE --username jachermocilla
tar czvf ics-os-$DTE.tar.gz ics-os-$DTE/
rm -fr ics-os-$DTE/
