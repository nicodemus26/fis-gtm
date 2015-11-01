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

bool gvcmx_get(v)
mval *v;
{
	mval temp;

	temp.mvtype = 0;
	gvcmz_doop (CMMS_Q_GET,CMMS_R_GET, &temp);
	if (MV_DEFINED(&temp))
		*v = temp;

	return MV_DEFINED(&temp);
}
