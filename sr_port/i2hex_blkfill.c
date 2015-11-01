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

void i2hex_blkfill(int num, uchar_ptr_t addr, int len)
{
	unsigned char	buff[8];
	int		i;

	assert(sizeof(buff) >= len);
	i2hex(num, buff, len);
	for ( i = 0; i < len - 1 ; i++)
	{
		if (buff[i] != '0')
			break;
		buff[i] = ' ';
	}
	memcpy(addr, buff, len);
}

void i2hexl_blkfill(qw_num num, uchar_ptr_t addr, int len)
{
	unsigned char	buff[16];
	int		i;

	assert(sizeof(buff) >= len);
	i2hexl(num, buff, len);
	for ( i = 0; i < len - 1 ; i++)
	{
		if (buff[i] != '0')
			break;
		buff[i] = ' ';
	}
	memcpy(addr, buff, len);
}
