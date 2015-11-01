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
 * -----------------------------------------------
 * op_fnp1 Piece function (the piecemaker)
 * Special case of 1 char delimiter and 1 piece (reference)
 *
 * Arguments:
 *	src		- pointer to Source mval
 *	del		- delimiter char to use looking for a piece
 *	trgpcidx	- index of piece to extract from source string
 *	dst		- pointer to Destination mval to save the piece in
 *
 * Return:
 *	none
 *
 * Side effects:
 *	dst structure gets filled with the result
 * Notes:
 *	this function is handled by an assembler (mar) module
 *	on the Vax.
 * -----------------------------------------------
 */


#include "mdef.h"

#ifndef __vax
#include "fnpc.h"
#include "gtm_stdio.h"
#include "min_max.h"
#include "op.h"

GBLREF fnpc_area	fnpca;			/* The $piece cache area */

#ifdef DEBUG
GBLREF	uint4	process_id;
GBLREF	boolean_t	setp_work;		/* The work we are doing is for set $piece */
GBLREF	int	c_miss;				/* cache misses (debug) */
GBLREF	int	c_hit;				/* cache hits (debug) */
GBLREF	int	c_small;			/* scanned small string brute force */
GBLREF	int	c_small_pcs;			/* chars scanned by small scan */
GBLREF	int	c_pskip;			/* number of pieces "skipped" */
GBLREF	int	c_pscan;			/* number of pieces "scanned" */
GBLREF	int	c_parscan;			/* number of partial scans (partial cache hits) */
GBLREF	int	cs_miss;			/* cache misses (debug) */
GBLREF	int	cs_hit;				/* cache hits (debug) */
GBLREF	int	cs_small;			/* scanned small string brute force */
GBLREF	int	cs_small_pcs;			/* chars scanned by small scan */
GBLREF	int	cs_pskip;			/* number of pieces "skipped" */
GBLREF	int	cs_pscan;			/* number of pieces "scanned" */
GBLREF	int	cs_parscan;			/* number of partial scans (partial cache hits) */
GBLREF	int	c_clear;			/* cleared due to (possible) value change */
#  define COUNT_EVENT(x) if (setp_work) ++cs_##x; else ++c_##x;
#  define INCR_COUNT(x,y) if (setp_work) cs_##x += y; else c_##x += y;
#else
#  define COUNT_EVENT(x)
#  define INCR_COUNT(x,y)
#endif

void op_fnp1(mval *src, int del, int trgpcidx,  mval *dst, boolean_t srcisliteral)
{
	unsigned char	*first, *last, *start, *end;
	unsigned char	dlmc;
	unsigned int	*pcoff, *pcoffmax, fnpc_indx, slen;

	int		trgpc, cpcidx, spcidx;
	fnpc   		*cfnpc;

	MV_FORCE_STR(src);
	dst->mvtype = MV_STR;
	dlmc = del;
	start = first = last = (unsigned char *)src->str.addr;
	slen = src->str.len;
	end = start + slen;

	/* Detect annoyance cases and deal with quickly so we don't muck up the
	   logic below trying to handle it properly */
	if (0 >= trgpcidx || 0 == slen)
	{
		dst->str.addr = (char *)start;
		dst->str.len  = 0;
		return;
	}

	/* If the string length meets our minimum requirements, lets play
	   "What's in my cache"  */
	if (FNPC_STRLEN_MIN < slen && !srcisliteral)
	{
		/* Test mval for valid cache: index ok, mval addr same, delim same */
		fnpc_indx = src->fnpc_indx - 1;
		cfnpc = &fnpca.fnpcs[fnpc_indx];
		if (FNPC_MAX > fnpc_indx && cfnpc->last_str.addr == (char *)first &&
		    cfnpc->last_str.len == slen && cfnpc->delim == del)
		{
			/* Have valid cache. See if piece we want already in cache */
			COUNT_EVENT(hit);
			INCR_COUNT(pskip, cfnpc->npcs);

			if (trgpcidx <= cfnpc->npcs)
			{
				/* Piece is totally in cache no scan needed */
				dst->str.addr = (char *)first + cfnpc->pstart[trgpcidx - 1];
				dst->str.len = cfnpc->pstart[trgpcidx] - cfnpc->pstart[trgpcidx - 1] - 1;
				assert(dst->str.len >= 0 && dst->str.len <= src->str.len);
				return;
			} else
			{
				/* Not in cache but pick up scan where we left off */
				cpcidx = cfnpc->npcs;
				first = last = start + cfnpc->pstart[cpcidx];	/* First char of next pc */
				pcoff = &cfnpc->pstart[cpcidx];
				if (pcoff == cfnpc->pcoffmax)
					++pcoff; 		/* No further updates to pstart array */
				++cpcidx;			/* Now past last piece and on to next one */
				COUNT_EVENT(parscan);
			}
		} else
		{
			/* The piece cache index or mval validation was incorrect.
			   Start from the beginning */

			COUNT_EVENT(miss);

			/* Need to steal a new piece cache, get "least recently reused" */
			cfnpc = fnpca.fnpcsteal;		/* Get next element to steal */
			if (fnpca.fnpcmax < cfnpc)
				cfnpc = &fnpca.fnpcs[0];
			fnpca.fnpcsteal = cfnpc + 1;		/* -> next element to steal */

			cfnpc->last_str = src->str;		/* Save validation info */
			cfnpc->delim = del;
			cfnpc->npcs = 0;
			src->fnpc_indx = cfnpc->indx + 1;	/* Save where we are putting this element
								   (1 based index in mval so 0 isn't so common) */
			pcoff = &cfnpc->pstart[0];
			cpcidx = 1;				/* current piece index */
		}

		/* Do scan filling in offsets of pieces if they fit in the cache */
		spcidx = cpcidx;				/* Starting value for search */
		pcoffmax = cfnpc->pcoffmax;			/* Local end of array value */
		while (cpcidx <= trgpcidx && last < end)
		{
			/* Once through for each piece we pass, last time through to find length of piece we want */
			first = last;				/* first char of current piece */
			while (last != end && *last != dlmc) last++;	/* Find delim signaling end of piece */
			last++;					/* Bump past delim to first char next piece,
								   or if hit last char, +2 past end of piece */
			++cpcidx;				/* Next piece */
			if (pcoff < pcoffmax)
				*pcoff++ = first - start;	/* offset to this piece */
			if (pcoff == pcoffmax)
				*pcoff++ = last - start;	/* store start of first piece beyond what is in cache */
		}

		dst->str.addr = (char *)first;

		/* If we scanned some chars, adjust end pointer and save end of final piece */
		if (spcidx != cpcidx)
		{
			if (pcoff < pcoffmax)
				*pcoff = last - start;		/* If not at end of cache, save start of "next" piece */

			--last;					/* Undo bump past last delim or +2 past end char
								   of piece for accurate string len */
			/* Update count of pieces in cache */
			cfnpc->npcs = MIN((cfnpc->npcs + cpcidx - spcidx), FNPC_ELEM_MAX);
			assert(cfnpc->npcs <= FNPC_ELEM_MAX);
			assert(cfnpc->npcs > 0);

			/* If we the above loop ended prematurely because we ran out of text, we return null string */
			if (trgpcidx < cpcidx)
				dst->str.len = last - first;	/* Length of piece we located */
			else
				dst->str.len = 0;

			INCR_COUNT(pscan, cpcidx - spcidx);	/* Pieces scanned */
		} else
			dst->str.len  = 0;

		assert(cfnpc->npcs > 0);
		assert(dst->str.len >= 0 && dst->str.len <= src->str.len);
	} else
	{
		/* We have too small a string to worry about cacheing with or the string is a literal for which we cannot
		   change the fnpc index. In the first case it would take more work to set up the cache than it would to
		   just scan it (several times). In the second case, linked images have their literal mvals in readonly
		   storage so any attempt to set cacheing info into the mval results in an ACCVIO so cacheing must be
		   bypassed.
		*/
		COUNT_EVENT(small);
		cpcidx = 0 ;
		while (cpcidx < trgpcidx && last < end)
		{
			first = last;
			while (last != end && *last != dlmc) last++ ;
			last++; cpcidx++;
		}
		last-- ;
		dst->mvtype = MV_STR;
		dst->str.addr = (char *)first;
		dst->str.len  = (cpcidx == trgpcidx && cpcidx != 0 ? last - first : 0);
		INCR_COUNT(small_pcs, cpcidx);
	}
	return;

}

#ifdef DEBUG
void print_fnpc_stats(void)
{
	fprintf(stderr, "process id:          %d\n", process_id);
	fprintf(stderr, "fnpc cache clears:   %d\n", c_clear);
	fprintf(stderr, "Reference Piece:\n");
	fprintf(stderr, "  fnpc cache miss:     %d\n", c_miss);
	fprintf(stderr, "  fnpc cache hit:      %d\n", c_hit);
	fprintf(stderr, "  fnpc pieces skipped: %d\n", c_pskip);
	fprintf(stderr, "  fnpc pieces scanned: %d\n", c_pscan);
	fprintf(stderr, "  fnpc partial scans:  %d\n", c_parscan);
	fprintf(stderr, "  small string scans:  %d\n", c_small);
	fprintf(stderr, "  small str pcs scnd:  %d\n", c_small_pcs);
	fprintf(stderr, "Set Piece:\n");
	fprintf(stderr, "  fnpc cache miss:     %d\n", cs_miss);
	fprintf(stderr, "  fnpc cache hit:      %d\n", cs_hit);
	fprintf(stderr, "  fnpc pieces skipped: %d\n", cs_pskip);
	fprintf(stderr, "  fnpc pieces scanned: %d\n", cs_pscan);
	fprintf(stderr, "  fnpc partial scans:  %d\n", cs_parscan);
	fprintf(stderr, "  small string scans:  %d\n", cs_small);
	fprintf(stderr, "  small str pcs scnd:  %d\n", cs_small_pcs);
}
#endif
#endif
