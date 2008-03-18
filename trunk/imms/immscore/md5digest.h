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

        FILE *fp = fopen(filename.c_str(), "r");
        if (!fp)
            return "bad_checksum";

        long offset = OFFSET;
        fseek(fp, TAGOFFSET, SEEK_END);
        fread(tag_buf, 4, 1, fp);
        if (!strncmp(tag_buf, "TAG", 3))
            offset += TAGOFFSET;

        int res = fseek(fp, offset, SEEK_END);

        if (res)
            rewind(fp);

        int err = md5_stream(fp, NUMBLOCKS, bin_buffer);
        fclose(fp);
        if (err)
            return "bad_checksum";

        char *cur = hex_buf;
        for (int i = 0; i < (128 / 4 / 2); ++i)
            cur += sprintf(cur, "%02x", bin_buffer[i]);

        return hex_buf;
    }
};

#endif
