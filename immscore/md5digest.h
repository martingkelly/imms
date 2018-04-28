/*
 IMMS: Intelligent Multimedia Management System
 Copyright (C) 2001-2009 Michael Grigoriev

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef __MD5DIGEST_H
#define __MD5DIGEST_H

#include <string>
#include <stdio.h>
#include "md5.h"

#include <string.h>

using std::string;

#define NUMBLOCKS 256
#define BLOCKSIZE 4096
#define OFFSET (long) (- NUMBLOCKS * BLOCKSIZE)
#define TAGOFFSET (long) (-128)

class Md5Digest
{
public:
    static string digest_file(string filename)
    {
        static unsigned char bin_buffer[128 / 8];
        static char hex_buf[34] = {'\0'};
        static char tag_buf[4] = {'\0'};
        long offset = OFFSET;
        char *cur = hex_buf;
        int res, err;

        FILE *fp = fopen(filename.c_str(), "r");
        if (!fp)
            goto failed;

        err = fseek(fp, TAGOFFSET, SEEK_END);
        if (err)
            goto failed;

        err = fread(tag_buf, 4, 1, fp) != 1;
        if (err)
            goto failed;

        if (!strncmp(tag_buf, "TAG", 3))
            offset += TAGOFFSET;

        res = fseek(fp, offset, SEEK_END);
        if (res)
            rewind(fp);

        err = md5_stream(fp, NUMBLOCKS, bin_buffer);
        if (err)
            goto failed;

        err = fclose(fp);
        fp = NULL;
        if (err)
            goto failed;

        for (int i = 0; i < (128 / 4 / 2); ++i)
            cur += sprintf(cur, "%02x", bin_buffer[i]);

        return hex_buf;

failed:
        LOG(ERROR) << "Error hashing file \"" << filename << "\": " << error_string(errno) << endl;
        if (fp) {
            err = fclose(fp);
            fp = NULL;
            if (err)
                goto failed;
        }
        return "bad_checksum";
    }
};

#endif
