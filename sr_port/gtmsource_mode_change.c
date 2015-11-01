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

#if !defined(__MVS__) && !defined(VMS)
#include <sys/param.h>
#endif
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include "gtm_unistd.h"
#include <netinet/in.h> /* Required for gtmsource.h */
#include <arpa/inet.h>
#include "gtm_string.h"
#ifdef UNIX
#include <sys/sem.h>
#endif
#ifdef VMS
#include <descrip.h> /* Required for gtmsource.h */
#endif

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "repl_dbg.h"
#include "gtm_stdio.h"
#include "repl_shutdcode.h"
#include "repl_sem.h"
#include "repl_log.h"

GBLREF	jnlpool_addrs		jnlpool;
GBLREF	gtmsource_options_t	gtmsource_options;
GBLREF	boolean_t		update_disable;

int gtmsource_mode_change(int to_mode)
{
	uint4		savepid;
	int		exit_status;
	int		status, detach_status, remove_status;

	repl_log(stdout, FALSE, FALSE, "Initiating change of mode from %s to %s\n", (GTMSOURCE_MODE_ACTIVE == to_mode) ?
			"PASSIVE" : "ACTIVE", (GTMSOURCE_MODE_ACTIVE == to_mode) ? "ACTIVE" : "PASSIVE");

	/* Grab the jnlpool jnlpool option write lock */
	if (0 > grab_sem(SOURCE, SRC_SERV_OPTIONS_SEM))
	{
		repl_log(stderr, FALSE, TRUE,
		  "Error grabbing jnlpool access control/jnlpool option write lock : %s. Could not change mode\n", REPL_SEM_ERROR);
		return (ABNORMAL_SHUTDOWN);
	}

	if (jnlpool.gtmsource_local->mode == to_mode)
	{
		repl_log(stderr, FALSE, TRUE, "Source Server already %s, not changing mode\n",
				(to_mode == GTMSOURCE_MODE_ACTIVE) ? "ACTIVE" : "PASSIVE");
		rel_sem(SOURCE, SRC_SERV_OPTIONS_SEM);
		return (ABNORMAL_SHUTDOWN);
	}

	if (GTMSOURCE_MODE_ACTIVE == to_mode)
	{
		jnlpool.gtmsource_local->secondary_port = gtmsource_options.secondary_port;
		jnlpool.gtmsource_local->secondary_inet_addr = gtmsource_options.sec_inet_addr;
		strcpy(jnlpool.gtmsource_local->secondary, gtmsource_options.secondary_host);
		jnlpool.gtmsource_local->connect_parms[GTMSOURCE_CONN_HARD_TRIES_COUNT] =
								gtmsource_options.connect_parms[GTMSOURCE_CONN_HARD_TRIES_COUNT];
		jnlpool.gtmsource_local->connect_parms[GTMSOURCE_CONN_HARD_TRIES_PERIOD] =
								gtmsource_options.connect_parms[GTMSOURCE_CONN_HARD_TRIES_PERIOD];
		jnlpool.gtmsource_local->connect_parms[GTMSOURCE_CONN_SOFT_TRIES_PERIOD] =
								gtmsource_options.connect_parms[GTMSOURCE_CONN_SOFT_TRIES_PERIOD];
		jnlpool.gtmsource_local->connect_parms[GTMSOURCE_CONN_ALERT_PERIOD] =
								gtmsource_options.connect_parms[GTMSOURCE_CONN_ALERT_PERIOD];
	}
	if ('\0' != gtmsource_options.log_file[0] &&
	    0 != strcmp(jnlpool.gtmsource_local->log_file, gtmsource_options.log_file))
	{
		strcpy(jnlpool.gtmsource_local->log_file, gtmsource_options.log_file);
		jnlpool.gtmsource_local->changelog = TRUE;
		repl_log(stdout, FALSE, TRUE, "Signalling change in log file from %s to %s\n",
				jnlpool.gtmsource_local->log_file, gtmsource_options.log_file);
	}

	jnlpool.gtmsource_local->mode = to_mode;
	grab_lock(jnlpool.jnlpool_dummy_reg);
	if (update_disable)
	{
		jnlpool.jnlpool_ctl->upd_disabled = TRUE;
		repl_log(stdout, FALSE, TRUE, "Updates are disabled now \n");
	}
	else
	{
		jnlpool.jnlpool_ctl->upd_disabled = FALSE;
		repl_log(stdout, FALSE, TRUE, "Updates are allowed now \n");
	}
	rel_lock(jnlpool.jnlpool_dummy_reg);

	rel_sem(SOURCE, SRC_SERV_OPTIONS_SEM);

	REPL_DPRINT1("Change mode signalled\n");

	return (NORMAL_SHUTDOWN);
}
