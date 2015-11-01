/****************************************************************
 *								*
 *	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"
#include "gtm_fcntl.h"
#include "gtm_unistd.h"
#include "gtm_stat.h"
#include "gtm_stdio.h"
#include "gtm_ulimit.h"
#include "gtm_time.h"

#include "error.h"
#include "gdsroot.h"
#include "gdsbt.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "gtmio.h"
#include "eintr_wrappers.h"
#include "rename_file_if_exists.h"
#include "util.h"
#include "gtmmsg.h"
#include "iosp.h"	/* for SS_NORMAL */

#define ZERO_SIZE	(128 * DISK_BLOCK_SIZE)

#define JNL_TIME_FORMAT	"_%Y%j%H%M%S"	/* %year%julienday%hours%minutes%seconds */

#define CHECK_AND_DEAL_WITH_ERROR_STATUS {					\
						if (0 != status)		\
						{				\
							info->status = status;	\
							close(fd);		\
							UNLINK(info->jnl);	\
							return EXIT_ERR;	\
						}				\
					}


uint4	cre_jnl_file (jnl_create_info *info)
{
	jnl_file_header	*header;
	jnl_record	*eof_record, *epoch_record;
	jrec_suffix	*eof_suffix, *epoch_suffix;
	struct stat	stat_buf;
	struct tm	*tm_struct;
	ulimit_t	length;
	char		jepoch_buff[DISK_BLOCK_SIZE];
	char		jeb_buff[EOF_RECLEN];
	union
	{
		jnl_file_header	jfh;
		char		jfh_block[HDR_LEN];
	} hdr_buffer;
	char		jnl_rename[JNL_NAME_SIZE];
	char		ctmp;
	char		*zeroes, *fn;
	int		fd, rename_len, status;
	unsigned int	fn_len;
	off_t		k, filepos, eof_rec_offset, zero_offset;
	size_t		writelen;
	int             stat_res, rename_stat;
	uint4		curr_time;

	info->status2 = SS_NORMAL; /* initialize secondary error status at start. this is a VMS only field used in portable code */
	assert(EPOCH_RECLEN < DISK_BLOCK_SIZE);
	/* rename is called not to overwrite the new journal file it will create */
        if (RENAME_FAILED == (rename_stat = rename_file_if_exists(info->jnl, info->jnl_len,
                					&info->status, jnl_rename, &rename_len)))
                return EXIT_ERR;
        else if (RENAME_NOT_REQD != rename_stat && info->prev_jnl_len)
        {
                memcpy(info->prev_jnl, jnl_rename, rename_len);
                info->prev_jnl[rename_len] = '\0';
                info->prev_jnl_len = rename_len;
        }
	OPENFILE3(info->jnl, O_CREAT | O_EXCL | O_RDWR, 0600, fd);
	if (-1 == fd)
	{
		info->status = errno;
		return EXIT_ERR;
	}
	eof_rec_offset = HDR_LEN;
	/* since short_time in jnl record is a uint4, and since JNL_SHORT_TIME expects time_t, we better ensure they are same.
	 * A change in the size of time_t would mean a redesign of the fields.
	 */
	assert(sizeof(time_t) == sizeof(uint4));
	JNL_SHORT_TIME(curr_time);
	if (info->before_images)
	{
		memset(jepoch_buff, 0, sizeof(jepoch_buff));

		/* Write an EPOCH record */
		epoch_record = (jnl_record *)jepoch_buff;
		epoch_record->jrec_type = JRT_EPOCH;
		epoch_record->jrec_backpointer = 0;

		epoch_record->val.jrec_epoch.pini_addr = 0;
		epoch_record->val.jrec_epoch.short_time = curr_time;
		epoch_record->val.jrec_epoch.tn = info->tn;
		QWASSIGN(epoch_record->val.jrec_epoch.jnl_seqno, info->reg_seqno);

		epoch_suffix = (jrec_suffix *)((char *)epoch_record + EPOCH_BACKPTR);
		epoch_suffix->backptr = EPOCH_BACKPTR;
		epoch_suffix->suffix_code = JNL_REC_TRAILER;

		LSEEKWRITE(fd, HDR_LEN, epoch_record, DISK_BLOCK_SIZE, status);
		CHECK_AND_DEAL_WITH_ERROR_STATUS;
		eof_rec_offset = HDR_LEN + DISK_BLOCK_SIZE;
	}
	/* Write a zero 512-byte block at the tail of the file to ensure "jb->filesize" gets properly picked up */
	assert(sizeof(jepoch_buff) == DISK_BLOCK_SIZE);	/* make sure we use a 512-byte buffer only */
	memset(jepoch_buff, 0, sizeof(jepoch_buff));
	assert(0 == eof_rec_offset % DISK_BLOCK_SIZE);
	assert(info->alloc > DIVIDE_ROUND_DOWN(eof_rec_offset, DISK_BLOCK_SIZE)); /* ensure we have space to write EOF record */
	zero_offset = ((off_t)info->alloc - 1) * DISK_BLOCK_SIZE;
	LSEEKWRITE(fd, zero_offset, jepoch_buff, DISK_BLOCK_SIZE, status);
	CHECK_AND_DEAL_WITH_ERROR_STATUS;

	/* Write an EOF record */
	eof_record = (jnl_record *)jeb_buff;
	eof_record->jrec_type = JRT_EOF;
	eof_record->jrec_backpointer = (info->before_images ? DISK_BLOCK_SIZE : 0);
	jnl_prc_vector(&eof_record->val.jrec_eof.process_vector);
	eof_record->val.jrec_eof.tn = info->tn;
	QWASSIGN(eof_record->val.jrec_eof.jnl_seqno, info->reg_seqno);
	eof_suffix = (jrec_suffix *)((char *)eof_record + EOF_BACKPTR);
	eof_suffix->backptr = EOF_BACKPTR;
	eof_suffix->suffix_code = JNL_REC_TRAILER;
	LSEEKWRITE(fd, eof_rec_offset, eof_record, EOF_RECLEN, status);
	CHECK_AND_DEAL_WITH_ERROR_STATUS;

	/* Write the file header */
	header = &hdr_buffer.jfh;
	memset(&hdr_buffer, 0, sizeof(hdr_buffer));
	memcpy(header->label, JNL_LABEL_TEXT, sizeof(JNL_LABEL_TEXT) - 1);
	memcpy(&header->who_created, &eof_record->val.jrec_eof.process_vector, sizeof(header->who_created));
	memcpy(&header->who_opened, &eof_record->val.jrec_eof.process_vector, sizeof(header->who_opened));
	header->end_of_data = eof_rec_offset;
	header->max_record_length = info->rsize;
	header->bov_timestamp = curr_time;
	header->eov_timestamp = curr_time;
	header->bov_tn = info->tn;
	header->eov_tn = info->tn;
	header->before_images = info->before_images;
	header->repl_state = info->repl_state;
	header->data_file_name_length = info->fn_len;
	memcpy(header->data_file_name, info->fn, info->fn_len);
	header->data_file_name[info->fn_len] = '\0';
	header->alignsize = info->alignsize;
	header->autoswitchlimit = info->autoswitchlimit;
	header->epoch_interval = info->epoch_interval;
	QWASSIGN(header->start_seqno, info->reg_seqno);
	header->prev_jnl_file_name_length = info->prev_jnl_len; ;
	memcpy(header->prev_jnl_file_name, info->prev_jnl, info->prev_jnl_len);
	header->jnl_alq = info->alloc;
	header->jnl_deq = info->extend;
	LSEEKWRITE(fd, 0, header, HDR_LEN, status);
	if (0 == status)
	{
		if (-1 == fchmod(fd, 0666))
			status = errno;
	}
	CHECK_AND_DEAL_WITH_ERROR_STATUS;
	close(fd);
	return EXIT_NRM;
}
