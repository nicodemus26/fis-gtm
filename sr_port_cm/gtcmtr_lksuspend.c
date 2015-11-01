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
#include "locklits.h"

GBLREF connection_struct *curr_entry;

bool gtcmtr_lksuspend()
{
	unsigned char *ptr;
	cm_region_list *reg_walk;

	ptr = curr_entry->clb_ptr->mbf;
	assert(*ptr == CMMS_L_LKSUSPEND);
	ptr++;

	reg_walk = curr_entry->region_root;
	while (reg_walk)
	{
		if (reg_walk->oper & (PENDING | COMPLETE))
			gtcml_lkbckout(reg_walk);
		reg_walk = reg_walk->next;
	}

	*curr_entry->clb_ptr->mbf = CMMS_M_LKSUSPENDED;
	curr_entry->clb_ptr->cbl  = 1;
	return TRUE;
}
