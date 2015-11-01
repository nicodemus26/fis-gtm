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

bool gvcmx_query()
{	mval v;

	gvcmz_doop(CMMS_Q_QUERY,CMMS_R_QUERY,&v);
	if (MV_FORCE_INT(&v))
		return TRUE;
	else
		return FALSE;
}
