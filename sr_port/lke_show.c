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

/*
 * -------------------------------------------------
 * lke_show.c : displays locks for qualified regions
 * used in    : lke.c
 * -------------------------------------------------
 */

#include "mdef.h"
#include "mlkdef.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "util.h"
#include "longcpy.h"
#include "gtcmtr_lke.h"
#include "lke.h"
#include "lke_getcli.h"

#define NOFLUSH 0
#define FLUSH	1
#define RESET	2

GBLREF	gd_region	*gv_cur_region;
GBLREF	gd_addr		*gd_header;
GBLREF	sgmnt_addrs	*cs_addrs;
GBLREF	short		crash_count;

error_def(ERR_NOREGION);


void	lke_show(void)
{
	bool			locks, all = TRUE, wait = TRUE, interactive = FALSE, match = FALSE, memory = TRUE, nocrit = TRUE;
	int4			pid, ls_len;
	int			n;
	char 			regbuf[MAX_RN_LEN], nodebuf[32], one_lockbuf[MAX_KEY_SZ];
	mlk_ctldata_ptr_t	ctl;
	mstr			reg, node, one_lock;


	/* Get all command parameters */
	reg.addr = regbuf;
	reg.len = sizeof regbuf;
	node.addr = nodebuf;
	node.len = sizeof nodebuf;
	one_lock.addr = one_lockbuf;
	one_lock.len = sizeof one_lockbuf;

	if (lke_getcli(&all, &wait, &interactive, &pid, &reg, &node, &one_lock, &memory, &nocrit) == 0)
		return;

	/* Search all regions specified on the command line */
	for (gv_cur_region = gd_header->regions, n = 0; n != gd_header->n_regions; ++gv_cur_region, ++n)
	{
		/* If region matches and is open */
		if ((reg.len == 0  ||
		     gv_cur_region->rname_len == reg.len  &&  memcmp(gv_cur_region->rname, reg.addr, reg.len) == 0)  &&
		    gv_cur_region->open)
		{
			match = TRUE;
			util_out_print("!/!AD!/", NOFLUSH, REG_LEN_STR(gv_cur_region));

			/* If distributed database, the region is located on another node */
			if (gv_cur_region->dyn.addr->acc_meth == dba_cm)
				/* Obtain lock info from the remote node */
				locks = gtcmtr_lke_showreq(gv_cur_region->dyn.addr->cm_blk, gv_cur_region->cmx_regnum,
							   all, wait, pid, &node);
			else
			{
				if (gv_cur_region->dyn.addr->acc_meth == dba_bg  || gv_cur_region->dyn.addr->acc_meth == dba_mm)
				{
					/* Local region */
					cs_addrs = &FILE_INFO(gv_cur_region)->s_addrs;
					ls_len = cs_addrs->lock_addrs[1] - cs_addrs->lock_addrs[0];
					ctl = (mlk_ctldata_ptr_t)malloc(ls_len);

					/* Prevent any modification of the lock space while we make a local copy of it */
					if (cs_addrs->critical != NULL)
						crash_count = cs_addrs->critical->crashcnt;
					if (!nocrit)
						grab_crit(gv_cur_region);
					longcpy((uchar_ptr_t)ctl, (uchar_ptr_t)cs_addrs->lock_addrs[0], ls_len);
					if (!nocrit)
						rel_crit(gv_cur_region);

					locks = ctl->blkroot == 0 ?
							FALSE:
							lke_showtree(NULL, (mlk_shrblk_ptr_t)R2A(ctl->blkroot), all, wait, pid,
													one_lock, memory);

					free(ctl);
				}
				else
				{
					util_out_print(NULL, RESET);
					util_out_print("Region is not BG, MM, or CM", FLUSH);
					locks = TRUE;
				}
			}

			if (!locks)
			{
				util_out_print(NULL, RESET);
				util_out_print("No locks were found in !AD", FLUSH, REG_LEN_STR(gv_cur_region));
			}
		}
	}

	if (!match  &&  reg.len != 0)
		rts_error(VARLSTCNT(4) ERR_NOREGION, 2, reg.len, reg.addr);

}
