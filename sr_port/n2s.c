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
#include "arit.h"
#include "stringpool.h"
#include "underr.h"

#define INT_DIGITS	10
#define PACKED_DIGITS	9

GBLREF spdesc	stringpool;

unsigned char *n2s(mval *mv_ptr)
{
	unsigned char	*start, *cp, *cp1;
	int4		exp, n0, m1, m0, tmp;
	unsigned char	lcl_buf[INT_DIGITS];

	if (!MV_DEFINED(mv_ptr))
	{
		underr(mv_ptr);
		return NULL;
	}
	start = stringpool.free;
	if (start + MAX_NUM_SIZE > stringpool.top)
	{
		stp_gcol(MAX_NUM_SIZE);
		start = stringpool.free;
	}
	cp = start;
	m1 = mv_ptr->m[1];
	if (m1 == 0)	/* SHOULD THIS BE UNDER THE MV_INT TEST? */
		*cp++ = '0';
	else  if (mv_ptr->mvtype & MV_INT)
	{
		if (m1 < 0)
		{
			*cp++ = '-';
			m1 = -m1;
		}
		cp1 = lcl_buf + sizeof(lcl_buf);
		/* m0 is the integer part */
		m0 = m1 / MV_BIAS;
		/* m1 will become the fractional part */
		m1 = m1 - (m0 * MV_BIAS);
		if (m1 > 0)
		{
			for (n0 = 0;  n0 < MV_BIAS_PWR;  n0++)
			{
				tmp = m1;
				m1 /= 10;
				tmp -= (m1 * 10);
				if (tmp)
					break;
			}
			*--cp1 = tmp + '0';
			for (n0++;  n0 < MV_BIAS_PWR;  n0++)
			{
				tmp = m1;
				m1 /= 10;
				*--cp1 = tmp - (m1 * 10) + '0';
			}
			*--cp1 = '.';
		}
		while (m0 > 0)
		{
			tmp = m0;
			m0 /= 10;
			*--cp1 = tmp - (m0 * 10) + '0';
		}
		n0 = &lcl_buf[sizeof(lcl_buf)] - cp1;
		memcpy(cp, cp1, n0);
		cp += n0;
	} else
	{
		exp = (int4)mv_ptr->e - MV_XBIAS;
		if (mv_ptr->sgn)
			*cp++ = '-';
		m0 = mv_ptr->m[0];
		if (exp < 0)
		{
			*cp++ = '.';
			for (n0 = exp;  n0 < 0;  n0++)
				*cp++ = '0';
		}
		for (;  m1;  m1 = m0, m0 = 0)
		{
			for (n0 = 0;  n0 < PACKED_DIGITS;  n0++)
			{
				if (exp-- == 0)
				{
					if (m0 == 0 && m1 == 0)
						break;
					*cp++ = '.';
				} else  if (exp < 0 && m0 == 0 && m1 == 0)
					break;
				tmp = m1 / MANT_LO;
				m1 = (m1 - tmp * MANT_LO) * 10;
				*cp++ = tmp + '0';
			}
		}
		while (exp-- > 0)
			*cp++ = '0';
	}
	mv_ptr->mvtype |= MV_STR;
	mv_ptr->mvtype &= ~MV_NUM_APPROX;
	mv_ptr->str.addr = (char *)start;
	mv_ptr->str.len = cp - start;
	stringpool.free = cp;
	return cp;
}
