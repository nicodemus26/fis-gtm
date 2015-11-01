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

/* Routines to do database writes -- both protected and not proctected from timer interrupts */

#include "mdef.h"

#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "gtm_stdio.h"
#include "db_write.h"

#define MAX_WRITE_RETRY 2

GBLREF	sigset_t	blockalrm;

int4	db_write (int4 fdesc, off_t fptr, sm_uc_ptr_t fbuff, size_t fbuff_len)
{
	sigset_t	savemask;
	int4		save_errno, retry_count;
	int4		status;

	/* Block SIGALRM signal - no timers can pop and give us trouble */
	sigprocmask(SIG_BLOCK, &blockalrm, &savemask);

	save_errno = 0;
	retry_count = MAX_WRITE_RETRY;
	/*
	 * The check below for EINTR need not be converted into an EINTR
	 * wrapper macro since there are other conditions and a retry
	 * count.
	 */
	do
	{
		if (-1 != lseek(fdesc, fptr, SEEK_SET))
			status = write(fdesc, fbuff, fbuff_len);
		else
			status = -1;
	} while (((-1 == status && EINTR == errno) || (status != fbuff_len)) && 0 < retry_count--);

	if (-1 == status)	    	/* Had legitimate error - return it */
		save_errno = errno;
	else if (status != fbuff_len)
		save_errno = -1;	/* Something kept us from getting what we wanted */

	/* Reset signal handlers  */
	sigprocmask(SIG_SETMASK, &savemask, NULL);

	return save_errno;
}

/* Database write routine not protected from timer incursion. This routine should only be
   used by wcs_wtstart which is known to protect itself against recursion due to timer
   pops. Otherwise, the danger is that a timer will go off in between the lseek and the write
   causing the write to put the data in an unintended place. */
int4	db_write_ntp (int4 fdesc, off_t fptr, sm_uc_ptr_t fbuff, size_t fbuff_len)
{
	int4		save_errno, retry_count;
	int4		status;

	save_errno = 0;
	retry_count = MAX_WRITE_RETRY;
	/*
	 * The check below for EINTR need not be converted into an EINTR
	 * wrapper macro since there are other conditions and a retry
	 * count.
	 */
	do
	{
		if (-1 != lseek(fdesc, fptr, SEEK_SET))
			status = write(fdesc, fbuff, fbuff_len);
		else
			status = -1;
	} while (((-1 == status && EINTR == errno) || (status != fbuff_len)) && 0 < retry_count--);

	if (-1 == status)	    	/* Had legitimate error - return it */
		save_errno = errno;
	else if (status != fbuff_len)
		save_errno = -1;	/* Something kept us from getting what we wanted */

	return save_errno;
}
