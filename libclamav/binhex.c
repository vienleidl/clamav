/*
 *  Copyright (C) 2004 Nigel Horne <njh@bandsman.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Change History:
 * $Log: binhex.c,v $
 * Revision 1.2  2004/11/18 18:09:06  nigelhorne
 * First draft of binhex.c
 *
 */
static	char	const	rcsid[] = "$Id: binhex.c,v 1.2 2004/11/18 18:09:06 nigelhorne Exp $";

#include "clamav.h"

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#ifndef	CL_DEBUG
#define	NDEBUG	/* map CLAMAV debug onto standard */
#endif

#ifdef CL_THREAD_SAFE
#ifndef	_REENTRANT
#define	_REENTRANT	/* for Solaris 2.8 */
#endif
#endif

#if HAVE_MMAP
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#else /* HAVE_SYS_MMAN_H */
#undef HAVE_MMAP
#endif
#endif

#include <stdio.h>
#include <memory.h>
#include <sys/stat.h>
#include "line.h"
#include "mbox.h"
#include "table.h"
#include "blob.h"
#include "text.h"
#include "others.h"

int
cli_binhex(const char *dir, int desc)
{
	struct stat statb;
	char *buf, *start;
	size_t size, bytesleft;
	message *m;
	fileblob *fb;

#ifndef HAVE_MMAP
	cli_errmsg("Binhex decoding needs mmap() (for now)\n");
	return CL_EMEM;
#else
	if(fstat(desc, &statb) < 0)
		return CL_EOPEN;

	m = messageCreate();
	if(m == NULL)
		return CL_EMEM;

	size = statb.st_size;
	start = buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, desc, 0);
	if (buf == MAP_FAILED)
		return CL_EMEM;

	cli_dbgmsg("mmap'ed binhex file\n");

	bytesleft = size;

	while(bytesleft) {
		int length = 0;
		char *ptr, *line;

		for(ptr = buf; bytesleft && *ptr != '\r'; ptr++) {
			length++;
			--bytesleft;
		}

		printf("%d: ", length);

		line = cli_malloc(length + 1);

		memcpy(line, buf, length);
		line[length] = '\0';

		puts(line);

		if(messageAddStr(m, line) < 0)
			break;

		free(line);

		buf = ++ptr;
	}
	munmap(start, size);

	if(m->binhex == NULL) {
		messageDestroy(m);
		cli_errmsg("No binhex line found\n");
		return CL_EFORMAT;
	}
	messageSetEncoding(m, "x-binhex");

	fb = messageToFileblob(m, dir);
	if(fb) {
		cli_dbgmsg("Binhex file decoded to %s\n", fileblobGetFilename(fb));
		fileblobDestroy(fb);
	} else
		cli_errmsg("Couldn't decode binhex file to %s\n", fileblobGetFilename(fb));
	messageDestroy(m);

	if(fb)
		return CL_CLEAN;	/* a lie - but it gets things going */
	return CL_EOPEN;
#endif
}
