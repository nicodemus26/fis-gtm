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

	.TITLE	mum_tstart.s

.include	"linkage.si"
	.include	"g_msf.si"
#comment perhaps
	.sbttl	mum_tstart
	.data
.extern	frame_pointer
.extern	proc_act_type

	.text
.extern	trans_code
.extern	inst_flush

ENTRY mum_tstart
	addl	$4,%esp		# back up over return address
	cmpb	$0,proc_act_type
	je	l1
	call	trans_code
l1:	getframe
	call	inst_flush	# smw 99/11/24 is this needed
	ret
