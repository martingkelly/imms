#include "song.h"
#include "md5digest.h"
#include "sqlite++.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

using std::cerr;
using std::endl;

Song Song::identify(const string &path)
{
    Song song;

    struct stat statbuf;
    if (stat(path.c_str(), &statbuf))
        return Song();

    time_t modtime = statbuf.st_mtime;

    try {
        AutoTransaction a;

        {
            Q q("SELECT uid, sid, modtime FROM 'Library' WHERE path = ?;");
            q << path;

            if (q.next())
            {
                time_t last_modtime;
                q >> song.uid >> song.sid >> last_modtime;

                if (modtime == last_modtime)
                    return song;
            }
        }

        string checksum = Md5Digest::digest_file(path);

        // old path but modtime has changed - update checksum
        if (song.uid != -1)
        {
            Q q("UPDATE 'Library' SET modtime = ?, "
                    "checksum = ? WHERE path = ?';");
            q << modtime << checksum << path;
            q.execute();
            a.commit();
            return song;
        }

        // moved or new file and path needs updating
        song.reset();

        Q q("SELECT uid, sid, path FROM 'Library' WHERE checksum = ?;");
        q << checksum;

        if (q.next())
        {
            // Check if any of the old paths no longer exist 
            // (aka file was moved) so that we can reuse their uid
            do
            {
                string oldpath;
                q >> song.uid >> song.sid >> oldpath;

                if (access(oldpath.c_str(), F_OK))
                {
                    q.reset();

                    {
                        Q q("UPDATE 'Library' SET sid = -1, "
                                "path = ?, modtime = ? WHERE path = ?;");

                        q << path << modtime << oldpath;

                        q.execute();
                    }
#ifdef DEBUG
                    cerr << "identify: moved: uid = " << song.uid << endl;
#endif
                    a.commit();
                    return song;
                }
            } while (q.next());
        }
        else
        {
            // figure out what the next uid should be
            Q q("SELECT max(uid) FROM Library;");
            if (q.next())
                q >> song.uid;
            ++song.uid;
        }

        {
            // new file - insert into the database
            Q q("INSERT INTO 'Library' "
                    "('uid', 'sid', 'path', 'modtime', 'checksum') "
                    "VALUES (?, -1, ?, ?, ?);");

            q << song.uid << path << modtime << checksum;

            q.execute();
        }

#ifdef DEBUG
        cerr << "identify: new: uid = " << song.uid << endl;
#endif

        a.commit();
        return song;
    }
    WARNIFFAILED();

    return Song();
}
