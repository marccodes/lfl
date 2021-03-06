/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "test.h"
__FBSDID("$FreeBSD: src/usr.bin/cpio/test/test_option_L.c,v 1.2 2008/08/24 06:21:00 kientzle Exp $");

#if defined(_WIN32) && !defined(__CYGWIN__)
#define CAT "type"
#else
#define CAT "cat"
#endif

DEFINE_TEST(test_option_L_upper)
{
	struct stat st;
	int fd, filelist;
	int r;

	filelist = open("filelist", O_CREAT | O_WRONLY, 0644);

	/* Create a file and a symlink to the file. */
	fd = open("file", O_CREAT | O_WRONLY, 0644);
	assert(fd >= 0);
	assertEqualInt(10, write(fd, "123456789", 10));
	close(fd);
	write(filelist, "file\n", 5);

	/* Symlink to above file. */
	assertEqualInt(0, symlink("file", "symlink"));
	write(filelist, "symlink\n", 8);

	close(filelist);

	r = systemf(CAT " filelist | %s -pd copy >copy.out 2>copy.err", testprog);
	assertEqualInt(r, 0);
	assertEqualInt(0, lstat("copy/symlink", &st));
#if !defined(_WIN32) || defined(__CYGWIN__)
	failure("Regular -p without -L should preserve symlinks.");
	assert(S_ISLNK(st.st_mode));
#endif

	r = systemf(CAT " filelist | %s -pd -L copy-L >copy-L.out 2>copy-L.err", testprog);
	assertEqualInt(r, 0);
	assertEmptyFile("copy-L.out");
	assertTextFileContents("1 block\n", "copy-L.err");
	assertEqualInt(0, lstat("copy-L/symlink", &st));
	failure("-pdL should dereference symlinks and turn them into files.");
	assert(!S_ISLNK(st.st_mode));

	r = systemf(CAT " filelist | %s -o >archive.out 2>archive.err", testprog);
	failure("Error invoking %s -o ", testprog);
	assertEqualInt(r, 0);

	assertEqualInt(0, mkdir("unpack", 0755));
#if defined(_WIN32) && !defined(__CYGWIN__)
	assertEqualInt(0, chdir("unpack"));
	r = systemf("type ..\\archive.out | %s -i >unpack.out 2>unpack.err", testprog);
	assertEqualInt(0, chdir(".."));
#else
	r = systemf("cat archive.out | (cd unpack ; %s -i >unpack.out 2>unpack.err)", testprog);
#endif
	failure("Error invoking %s -i", testprog);
	assertEqualInt(r, 0);
	assertEqualInt(0, lstat("unpack/symlink", &st));
#if !defined(_WIN32) || defined(__CYGWIN__)
	assert(S_ISLNK(st.st_mode));
#endif

	r = systemf(CAT " filelist | %s -oL >archive-L.out 2>archive-L.err", testprog);
	failure("Error invoking %s -oL", testprog);
	assertEqualInt(r, 0);

	assertEqualInt(0, mkdir("unpack-L", 0755));
#if defined(_WIN32) && !defined(__CYGWIN__)
	assertEqualInt(0, chdir("unpack-L"));
	r = systemf("type ..\\archive-L.out | %s -i >unpack-L.out 2>unpack-L.err", testprog);
	assertEqualInt(0, chdir(".."));
#else
	r = systemf("cat archive-L.out | (cd unpack-L ; %s -i >unpack-L.out 2>unpack-L.err)", testprog);
#endif
	failure("Error invoking %s -i < archive-L.out", testprog);
	assertEqualInt(r, 0);
	assertEqualInt(0, lstat("unpack-L/symlink", &st));
	assert(!S_ISLNK(st.st_mode));
}
