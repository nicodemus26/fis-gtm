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
#include "gdsblk.h"
#include "gdskill.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gdscc.h"
#include "io.h"
#include "iottdef.h"
#include "jnl.h"
#include "rtnhdr.h"
#include "stack_frame.h"
#include "stringpool.h"
#include "svnames.h"
#include "hashtab.h"		/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "error.h"
#include "op.h"
#include "mvalconv.h"
#include "zroutines.h"
#include "getstorage.h"
#include "get_command_line.h"
#include "getzposition.h"
#include "getzprocess.h"
#include "get_ret_targ.h"
#include "dollar_zlevel.h"
#include "gtmmsg.h"


#define ESC_OFFSET		4
#define MAX_COMMAND_LINE_LENGTH	255

GBLDEF mval		dollar_zmode;
GBLDEF mval		dollar_zproc;
GBLDEF mstr		dollar_zroutines;
GBLDEF mstr		dollar_zcompile;

GBLREF mval		dollar_zdir;
GBLREF stack_frame	*frame_pointer;
GBLREF spdesc		stringpool;
GBLREF io_pair		io_curr_device;
GBLREF io_log_name	*io_root_log_name;
GBLREF io_log_name	*dollar_principal;
GBLREF mval		dollar_ztrap;
GBLREF mval		dollar_zgbldir;
GBLREF mval		dollar_job;
GBLREF mval		dollar_zstatus;
GBLREF mval		dollar_zstep;
GBLREF char		*zro_root;  /* ACTUALLY some other pointer type! */
GBLREF mstr		gtmprompt;
GBLREF int		dollar_zmaxtptime;
GBLREF mstr		dollar_zsource;
GBLREF int4		dollar_zsystem;
GBLREF int4		dollar_zcstatus;
GBLREF int4		dollar_zeditor;
GBLREF short		dollar_tlevel;
GBLREF short		dollar_trestart;
GBLREF mval		dollar_ecode;
GBLREF mval		dollar_etrap;
GBLREF mval		dollar_zerror;
GBLREF mval		dollar_zyerror;

LITREF mval		literal_zero,literal_one;
LITREF char		gtm_release_name[];
LITREF int4		gtm_release_name_len;

void op_svget(int varnum, mval *v)
{
	io_log_name	*tl;
	int 		count;
	char		*c1, *c2;

	error_def(ERR_UNIMPLOP);
	error_def(ERR_TEXT);

	switch (varnum)
	{
	case SV_HOROLOG:
		op_horolog(v);
		break;
	case SV_ZGBLDIR:
		v->mvtype = MV_STR;
		v->str = dollar_zgbldir.str;
		break;
	case SV_PRINCIPAL:
		tl = dollar_principal ? dollar_principal : io_root_log_name->iod->trans_name;
		v->str.addr = tl->dollar_io;
		v->str.len = tl->len;
		/*** The following should be in the I/O code ***/
		if (ESC == *v->str.addr)
		{
			if (5 > v->str.len)
				v->str.len = 0;
			else
			{
				v->str.addr += ESC_OFFSET;
				v->str.len -= ESC_OFFSET;
			}
		}
		s2pool(&(v->str));
		v->mvtype = MV_STR;
		break;
	case SV_ZIO:
		v->mvtype = MV_STR;
		/* NOTE:	This is **NOT** equivalent to :
		 *		io_curr_log_name->dollar_io
		 */
		v->str.addr = io_curr_device.in->trans_name->dollar_io;
		v->str.len = io_curr_device.in->trans_name->len;
		if (ESC == *v->str.addr)
		{
			if (5 > v->str.len)
				v->str.len = 0;
			else
			{	v->str.addr += ESC_OFFSET;
				v->str.len -= ESC_OFFSET;
			}
		}
		s2pool(&(v->str));
		break;
	case SV_JOB:
		*v = dollar_job;
		break;
	case SV_STORAGE:
		i2mval(v, getstorage());
		break;
	case SV_TLEVEL:
		count = (int)dollar_tlevel;
		MV_FORCE_MVAL(v, count);
		break;
	case SV_TRESTART:
		MV_FORCE_MVAL(v, (int)((MAX_VISIBLE_TRESTART < dollar_trestart) ? MAX_VISIBLE_TRESTART : dollar_trestart));
		break;
	case SV_X:
		count = (int)io_curr_device.out->dollar.x;
		MV_FORCE_MVAL(v, count);
		break;
	case SV_Y:
		count = (int)io_curr_device.out->dollar.y;
		MV_FORCE_MVAL(v, count);
		break;
	case SV_ZA:
		count = (int)io_curr_device.in->dollar.za;
		MV_FORCE_MVAL(v, count);
		break;
	case SV_ZB:
		c1 = (char *)io_curr_device.in->dollar.zb;
		c2 = c1 + sizeof(io_curr_device.in->dollar.zb);
		if (sizeof(io_curr_device.in->dollar.zb) > stringpool.top - stringpool.free)
			stp_gcol(sizeof(io_curr_device.in->dollar.zb));
		v->mvtype = MV_STR;
		v->str.addr = (char *)stringpool.free;
		while (c1 < c2 && *c1)
			*stringpool.free++ = *c1++;
		v->str.len = (char *)stringpool.free - v->str.addr;
		break;
	case SV_ZC:	/****THESE ARE DUMMY VALUES ONLY!!!!!!!!!!!!!!!!!****/
		MV_FORCE_MVAL(v, 0);
		break;
	case SV_ZCMDLINE:
		get_command_line(v);
		break;
	case SV_ZEOF:
		*v = io_curr_device.in->dollar.zeof ? literal_one : literal_zero;
		break;
	case SV_IO:
		v->str.addr = io_curr_device.in->name->dollar_io;
		v->str.len = io_curr_device.in->name->len;
		/*** The following should be in the I/O code ***/
		if (ESC == *v->str.addr)
		{
			if (5 > v->str.len)
				v->str.len = 0;
			else
			{	v->str.addr += ESC_OFFSET;
				v->str.len -= ESC_OFFSET;
			}
		}
		s2pool(&(v->str));
		v->mvtype = MV_STR;
		break;
	case SV_PROMPT:
		v->mvtype = MV_STR;
		v->str.addr = gtmprompt.addr;
		v->str.len = gtmprompt.len;
		s2pool(&v->str);
		break;
	case SV_ZCOMPILE:
		v->mvtype = MV_STR;
		v->str = dollar_zcompile;
		s2pool(&(v->str));
		break;
	case SV_ZDIR:
		*v = dollar_zdir;
		break;
	case SV_ZSTEP:
		*v = dollar_zstep;
		break;
	case SV_ZMODE:
		*v = dollar_zmode;
		break;
	case SV_ZMAXTPTIME:
		i2mval(v, dollar_zmaxtptime);
		break;
	case SV_ZPOS:
		getzposition(v);
		break;
	case SV_ZPROC:
		getzprocess();
		*v = dollar_zproc;
		break;
	case SV_ZLEVEL:
		count = dollar_zlevel();
		MV_FORCE_MVAL(v, count);
		break;
	case SV_ZROUTINES:
		if (!zro_root)
			zro_init();
		v->mvtype = MV_STR;
		v->str = dollar_zroutines;
		s2pool(&(v->str));
		break;
	case SV_ZSOURCE:
		v->mvtype = MV_STR;
		v->str = dollar_zsource;
		break;
	case SV_ZSTATUS:
		*v = dollar_zstatus;
		s2pool(&(v->str));
		break;
	case SV_ZTRAP:
		v->mvtype = MV_STR;
		v->str = dollar_ztrap.str;
		s2pool(&(v->str));
		break;
	case SV_DEVICE:
		get_dlr_device(v);
		break;
	case SV_KEY:
		get_dlr_key(v);
		break;
	case SV_ZVERSION:
		v->mvtype = MV_STR;
		v->str.addr = &gtm_release_name[0];
		v->str.len = gtm_release_name_len;
		break;
	case SV_ZSYSTEM:
		MV_FORCE_MVAL(v, dollar_zsystem);
		break;
	case SV_ZCSTATUS:
		MV_FORCE_MVAL(v, dollar_zcstatus);
		break;
	case SV_ZEDITOR:
		MV_FORCE_MVAL(v, dollar_zeditor);
		break;
	case SV_QUIT:
		MV_FORCE_MVAL(v, (NULL == get_ret_targ()) ? 0 : 1);
		break;
	case SV_ECODE:
		v->mvtype = MV_STR;
		v->str = dollar_ecode.str;
		s2pool(&(v->str));
		break;
	case SV_ESTACK:
		gtm_putmsg(VARLSTCNT(1) MAKE_MSG_WARNING(ERR_UNIMPLOP));
		gtm_putmsg(VARLSTCNT(4) ERR_TEXT, 2, LEN_AND_LIT("$ESTACK"));
		MV_FORCE_MVAL(v, 0); /* till 'tis implemented */
		break;
	case SV_ETRAP:
		v->mvtype = MV_STR;
		v->str = dollar_etrap.str;
		s2pool(&(v->str));
		break;
	case SV_STACK:
		count = dollar_zlevel() - 1;
		MV_FORCE_MVAL(v, count);
		break;
	case SV_ZERROR:
		v->mvtype = MV_STR;
		v->str = dollar_zerror.str;
		s2pool(&(v->str));
		break;
	case SV_ZYERROR:
		v->mvtype = MV_STR;
		v->str = dollar_zyerror.str;
		s2pool(&(v->str));
		break;
	default:
		GTMASSERT;
	}
}
