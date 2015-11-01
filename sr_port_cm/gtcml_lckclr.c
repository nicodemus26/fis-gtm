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
#include "mlkdef.h"

GBLREF short cm_cmd_lk_ct;
GBLREF mlk_pvtblk *mlk_cm_root;

void gtcml_lckclr()
{
	short i;
	mlk_pvtblk *p1;

	p1 = mlk_cm_root;
	for (i = 0; i < cm_cmd_lk_ct; i++)
	{
		p1->trans = 0;
		p1 = p1->next;
	}
}
