#include "server.h"

#include "strmanip.h"

ImmsServer::ImmsServer()
    : SocketServer(string(getenv("HOME")).append("/.imms/socket"))
{
    conn = 0;
}

string consume(list<string> &l)
{
    string res;
    while (!l.empty())
    {
        res += l.front() + " ";
        l.pop_front();
    }
    return res;
}

string getnum(list<string> &l)
{
    int size = l.size();
    if (size < 1 || l.size() > 2)
        return "";

    string sign, num;
    if (size == 2)
    {
        sign = l.front();
        l.pop_front();
        num = l.front();
        l.pop_front();
    }
    else
    {
        sign = l.front().substr(0, 1);
        num = l.front().substr(1);
        l.pop_front();
    }

    if (sign != ">" && sign != "<" && sign != "=")
        return "";

    if (!atoi(num.c_str()))
        return "";

    return sign + " '" + num + "'";
}

void ImmsServer::do_events()
{
    int fd = accept();
    if (fd > 0)
    {
        delete conn;
        conn = new Socket(fd);
        *conn << "Welcome to " << PACKAGE_STRING << "\n";
        *conn << "This feature is highly experimental, "
            "so quit complaining" << "\n";
        *conn << "say 'help' if you are lost and hungry" << "\n";
    }

    if (conn && !conn->isok())
    {
        delete conn;
        conn = 0;
    }

    if (conn)
    {
        string command = conn->read();
        command = string_tolower(string_delete(command, "\n"));
        if (command == "")
            return;

        list<string> parsed;
        string_split(parsed, command, " ");

        while (1)
        {
            string str = parsed.front();
            parsed.pop_front();

            if (str == "help")
            {
                *conn << "known commands:" << "\n";
                *conn << "  show" << "\n";
                *conn << "  clear" << "\n";
                *conn << "  [or|and] artist <str>" << "\n";
                *conn << "  [or|and] rating [<|>] <int>" << "\n";
                *conn << "  [or|and] bpm [<|>] <int>" << "\n";
                *conn << "  sql <str>" << "\n";

                break;
            }
            else if (str == "show" || str == "ls" || str == "filter")
            {
                *conn << "filter: " << filter << "\n";
                break;
            }
            else if (str == "clear" || str == "reset")
            {
                filter = "";
                immsdb.install_filter(filter);
                break;
            }
            else if (str ==  "sql")
            {
                if (parsed.front() == "=")
                    parsed.pop_front();
                filter = consume(parsed);
                int n = immsdb.install_filter(filter);
                *conn << itos(n) << " hits" << "\n";
                break;
            }
            else if (str == "artist")
            {
                if (parsed.empty())
                {
                    *conn << "artist: parameter required" << "\n";
                    return;
                }
                if (parsed.front() == "=")
                    parsed.pop_front();
                str = consume(parsed);
                filter = "similar(Info.artist, '"
                    + string_normalize(str) + "')";
                int n = immsdb.install_filter(filter);
                *conn << itos(n) << " hits" << "\n";
                break;
            }
            else if (str == "rating")
            {
                str = getnum(parsed);
                if (str == "")
                {
                    *conn << "rating: parse error" << "\n";
                    return;
                }

                filter = "Rating.rating " + str;
                int n = immsdb.install_filter(filter);
                *conn << itos(n) << " hits" << "\n";
                break;
            }
            else if (str == "bpm")
            {
                str = getnum(parsed);
                if (str == "")
                {
                    *conn << "bpm: parse error" << "\n";
                    return;
                }

                filter = "Acoustic.bpm " + str;
                int n = immsdb.install_filter(filter);
                *conn << itos(n) << " hits" << "\n";
                break;
            }
            else
            {
                *conn << "parse error at " << str << "\n";
                return;
            }
        }
    
        if (parsed.size())
            *conn << "warning: ignored after " << parsed.front() << "\n";

    }
}

ImmsServer::~ImmsServer()
{
    delete conn;
    close();
    unlink(string(getenv("HOME")).append("/.imms/socket").c_str());
}
