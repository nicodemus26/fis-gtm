#!/bin/sh
#################################################################
#								#
#	Copyright 2001 Sanchez Computer Associates, Inc.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

ext=.m
for i in $*
do
	base=`basename $i $ext`
	if test $i != $base
	then {
		newf=`echo $base | tr '[a-z]' '[A-Z]'`
		if test $base != $newf
		then {
			dir=`dirname $i`
			echo $dir/$i "---> " $dir/$newf$ext
			mv $i $dir/$newf$ext
		}
		fi
	}
	fi
done
