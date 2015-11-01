/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"
#include "cmidef.h"
#include "cmmdef.h"
#include "stringpool.h"

GBLREF struct NTD *ntd_root;
GBLREF spdesc stringpool;

void gvcmx_canremlk()
{
	uint4	status,count,buffer;
	unsigned char *ptr;
	error_def(ERR_BADSRVRNETMSG);
	struct CLB	*p;

	if (!ntd_root)
		return;

	buffer = 0;

	for ( p = (struct CLB *)RELQUE2PTR(ntd_root->cqh.fl); p != (struct CLB *)ntd_root ;
		p = (struct CLB *)RELQUE2PTR(p->cqe.fl))
	{
		if (((link_info*)(p->usr))->lck_info & REQUEST_PENDING)
			buffer += p->mbl;
	}

	if (stringpool.top < stringpool.free + buffer)
		stp_gcol(buffer);

	ptr = stringpool.free;

	for ( p = (struct CLB *)RELQUE2PTR(ntd_root->cqh.fl); p != (struct CLB *)ntd_root ;
		p = (struct CLB *)RELQUE2PTR(p->cqe.fl))
	{
		if (((link_info*)(p->usr))->lck_info & REQUEST_PENDING)
		{	p->mbf = ptr;
			ptr += p->mbl;
		}
	}
	gvcmz_int_lkcancel();
}
