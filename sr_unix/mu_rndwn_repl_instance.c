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

#include <sys/sem.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "gtmrecv.h"
#include "iosp.h"
#include "gtm_stdio.h"
#include "gtmio.h"
#include "gtm_string.h"
#include "repl_instance.h"
#include "gtm_logicals.h"
#include "repl_sem.h"
#include "mu_rndwn_replpool.h"
#include "mu_rndwn_repl_instance.h"
#include "mu_gv_cur_reg_init.h"
#include "gtm_sem.h"
#include "gtm_unistd.h"
#include "gtmmsg.h"

GBLREF	jnlpool_addrs		jnlpool;
GBLREF	recvpool_addrs		recvpool;
GBLREF	gd_region		*gv_cur_region;

#define TMP_BUF_LEN     50

/*
 * This will rundown a replication instance journal (and receiver) pool.
 *	Input Parameter:
 *		replpool_id of the instance. Instaname must be null terminated in replpool_id.
 * Returns :
 *	TRUE,  if successful.
 *	FALSE, otherwise.
 */
boolean_t mu_rndwn_repl_instance(replpool_identifier *replpool_id)
{
	boolean_t		flag1, flag2;
	char			*instname, shmid_buff[TMP_BUF_LEN];
	gd_region		*r_save;
	int			status = 0;
	repl_inst_fmt		repl_instance;
	static	gd_region	*reg = NULL;
	struct semid_ds		semstat;
	struct shmid_ds		shmstat;
	union semun		semarg;
	uchar_ptr_t		ret_ptr;
	unix_db_info		*udi;

	error_def(ERR_MUJPOOLRNDWNSUC);
	error_def(ERR_MURPOOLRNDWNSUC);
	error_def(ERR_MUJPOOLRNDWNFL);
	error_def(ERR_MURPOOLRNDWNFL);

	if (NULL == reg)
	{
		r_save = gv_cur_region;
		mu_gv_cur_reg_init();
		reg = gv_cur_region;
		gv_cur_region = r_save;
	}
	jnlpool.jnlpool_dummy_reg = reg;
	instname = replpool_id->instname;
	reg->dyn.addr->fname_len = strlen(instname);
	assert(0 == instname[reg->dyn.addr->fname_len]);
	memcpy((char *)reg->dyn.addr->fname, instname, reg->dyn.addr->fname_len + 1);
	udi = FILE_INFO(reg);
	udi->fn = (char *)reg->dyn.addr->fname;
	/*
	 * --------------------------
	 * First rundown Journal pool
	 * --------------------------
	 */
	/* Lock jnlpool using ftok semaphore */
	get_lock_jnlpool_ftok_sems(TRUE, TRUE);
	/* read jnlpool semaphore and shared memory id from instance file */
	repl_inst_get((char *)instname, &repl_instance);
	semarg.buf = &semstat;
	if (0 != repl_instance.jnlpool_semid)
		if ((-1 == semctl(repl_instance.jnlpool_semid, 0, IPC_STAT, semarg)) ||
	 		(semarg.buf->sem_ctime != repl_instance.jnlpool_semid_ctime))
			repl_instance.jnlpool_semid = 0;
	if (0 != repl_instance.jnlpool_shmid)
		if ((-1 == shmctl(repl_instance.jnlpool_shmid, IPC_STAT, &shmstat)) ||
	 		(shmstat.shm_ctime != repl_instance.jnlpool_shmid_ctime))
			repl_instance.jnlpool_shmid = 0;
	/* Rundown Journal pool */
	flag1 = mu_rndwn_replpool(replpool_id, repl_instance.jnlpool_semid, repl_instance.jnlpool_shmid);
	if (repl_instance.jnlpool_shmid)
	{
		ret_ptr = i2asc((uchar_ptr_t)shmid_buff, repl_instance.jnlpool_shmid);
		*ret_ptr = '\0';
		gtm_putmsg(VARLSTCNT(6) (flag1 ? ERR_MUJPOOLRNDWNSUC : ERR_MUJPOOLRNDWNFL),
			4, LEN_AND_STR(shmid_buff), LEN_AND_STR(replpool_id->instname));
	}
	/* Reset instance file for journal pool semid and shmid */
	repl_inst_jnlpool_reset();
	/* Release jnlpool ftok semaphore lock */
	rel_jnlpool_ftok_sems(TRUE, TRUE);

	/*
	 * --------------------------
	 * Now rundown Receivpool
	 * --------------------------
	 */
	/* For instnace is same for both receive and journal pool */
	recvpool.recvpool_dummy_reg = reg;
	if (0 != repl_instance.recvpool_semid)
		if ((-1 == semctl(repl_instance.recvpool_semid, 0, IPC_STAT, semarg)) ||
	 		(semarg.buf->sem_ctime != repl_instance.recvpool_semid_ctime))
			repl_instance.recvpool_semid = 0;
	if (0 != repl_instance.recvpool_shmid)
		if ((-1 == shmctl(repl_instance.recvpool_shmid, IPC_STAT, &shmstat)) ||
	 		(shmstat.shm_ctime != repl_instance.recvpool_shmid_ctime))
			repl_instance.recvpool_shmid = 0;
	/* Lock receivpool using ftok semaphore */
	get_lock_recvpool_ftok_sems(TRUE, TRUE);
	flag2 = mu_rndwn_replpool(replpool_id, repl_instance.recvpool_semid, repl_instance.recvpool_shmid);
	if (repl_instance.recvpool_shmid)
	{
		ret_ptr = i2asc((uchar_ptr_t)shmid_buff, repl_instance.recvpool_shmid);
		*ret_ptr = '\0';
		gtm_putmsg(VARLSTCNT(6) (flag2 ? ERR_MURPOOLRNDWNSUC : ERR_MURPOOLRNDWNFL),
			4, LEN_AND_STR(shmid_buff), LEN_AND_STR(replpool_id->instname));
	}
	/* Reset instance file for receive pool semid and shmid */
	repl_inst_recvpool_reset();
	/* Release recvpool ftok semaphore lock */
	rel_recvpool_ftok_sems(TRUE, TRUE);

	return (flag1 && flag2);
}
