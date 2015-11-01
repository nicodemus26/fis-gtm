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
#include "gtm_stdlib.h"
#include "job.h"

/*
 * -------------------------------------------------------------
 * Load the child environment into parameter structure
 * -------------------------------------------------------------
 */
void ojgetch_env(job_params_type *jparms)
{
/*
 * Pass all information about the job via shell's environment
 * The child will get those variables to obtain the info about the job.
 */


	jparms->gbldir.addr = GETENV(GBLDIR_ENV);
	if (jparms->gbldir.addr)
		jparms->gbldir.len = strlen(jparms->gbldir.addr);
	else
		jparms->gbldir.len = 0;


	jparms->input.addr = GETENV(IN_FILE_ENV);
	if (jparms->input.addr)
		jparms->input.len = strlen(jparms->input.addr);
	else
		jparms->input.len = 0;

	jparms->output.addr = GETENV(OUT_FILE_ENV);
	if (jparms->output.addr)
		jparms->output.len = strlen(jparms->output.addr);
	else
		jparms->output.len = 0;

	jparms->error.addr = GETENV(ERR_FILE_ENV);
	if (jparms->error.addr)
		jparms->error.len = strlen(jparms->error.addr);
	else
		jparms->error.len = 0;

	jparms->routine.addr = GETENV(ROUTINE_ENV);
	if (jparms->routine.addr)
		jparms->routine.len = strlen(jparms->routine.addr);
	else
		jparms->routine.len = 0;

	jparms->label.addr = GETENV(LABEL_ENV);
	if (jparms->label.addr)
		jparms->label.len = strlen(jparms->label.addr);
	else
		jparms->label.len = 0;

	jparms->directory.addr = GETENV(CWD_ENV);
	if (jparms->directory.addr)
		jparms->directory.len = strlen(jparms->directory.addr);
	else
		jparms->directory.len = 0;

	jparms->offset = ATOL(GETENV(OFFSET_ENV));
}


