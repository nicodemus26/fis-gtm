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

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "mlkdef.h"
#include "cdb_sc.h"
#include "jnl.h"
#include "gdscc.h"
#include "gdskill.h"
#include "hashtab.h"		/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "t_retry.h"
#include "mlk_shrblk_delete_if_empty.h"
#include "mlk_tree_wake_children.h"
#include "mlk_unlock.h"
#include "mlk_wake_pending.h"
#include "gvusr.h"
#include "tp_grab_crit.h"

GBLREF	int4 		process_id;
GBLREF	short		crash_count;
GBLREF	short		dollar_tlevel;
GBLREF	unsigned int	t_tries;
GBLREF	tp_region	*tp_reg_free_list;	/* Ptr to list of tp_regions that are unused */
GBLREF  tp_region	*tp_reg_list;		/* Ptr to list of tp_regions for this transaction */

void mlk_unlock(mlk_pvtblk *p)
{
	int			status;
	mlk_shrblk_ptr_t	d, pnt;
	mlk_ctldata_ptr_t	ctl;
	bool			stop_waking, was_crit;
	sgmnt_addrs		*csa;

	if (p->region->dyn.addr->acc_meth != dba_usr)
	{
		csa = &FILE_INFO(p->region)->s_addrs;

		d = p->nodptr;
		ctl = p->ctlptr;
		if (csa->critical)
			crash_count = csa->critical->crashcnt;

		if (0 < dollar_tlevel && !((t_tries < CDB_STAGNATE) || csa->now_crit))
			/* ... Final retry and this region not locked down */
		{
			/* make sure this region is in the list in case we end up retrying */
			insert_region(p->region, &tp_reg_list, &tp_reg_free_list, sizeof(tp_region));
			if (FALSE == tp_grab_crit(p->region))		/* Attempt lockdown now */
			{
				status = cdb_sc_needcrit;		/* avoid deadlock -- restart transaction */
				t_retry(status);
			}
		}
		if (FALSE == (was_crit = csa->now_crit))
			grab_crit(p->region);
		if (d->owner == process_id && p->sequence == d->sequence)
		{
			d->owner = 0;
			d->sequence = csa->hdr->trans_hist.lock_sequence++;
			stop_waking = (d->children ? mlk_tree_wake_children(ctl,
				(mlk_shrblk_ptr_t)R2A(d->children), p->region) : FALSE);
			for (  ; d ; d = pnt)
			{
				if (d->parent)
					pnt = (mlk_shrblk_ptr_t)R2A(d->parent);
				else
					pnt = 0;
				if (!stop_waking && d->pending)
				{
					mlk_wake_pending(ctl, d, p->region);
					stop_waking = TRUE;
				}
				else
					mlk_shrblk_delete_if_empty(ctl, d);
			}
		}
		if (FALSE == was_crit)
			rel_crit(p->region);
	} else	/* acc_meth == dba_usr */
		gvusr_unlock(p->total_length, &p->value[0], p->region);

	return;
}
