#include "comm.h"

#include <string>

#include <unistd.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/select.h>

using std::string;

int fd;

SocketClient *s;

void linehandler(char *line)
{
    if (line == 0 || !*line)
        return;
    if (!strcmp(line, "quit"))
        exit(0);
    s->write(line);
    add_history(line);
    free(line);
}

int main(int argc, char **argv)
{
    string name = string(getenv("HOME")).append("/.imms/socket");
    SocketClient socket(name);
    s = &socket;

    if (!s->isok())
        return -1;

    rl_callback_handler_install("", &linehandler);

    fd_set myfds;

    while (1)
    {
        FD_SET(0, &myfds);
        FD_SET(s->getfd(), &myfds);

        select(s->getfd() + 1, &myfds, 0, 0, 0);

        if (FD_ISSET(s->getfd(), &myfds))
        {
            string res = s->read();
            if (res != "")
            {
                printf("%s", res.c_str());
                fflush(stdout);
            }
        }
        else if (FD_ISSET(0, &myfds))
            rl_callback_read_char();
    }

    return 0;
}
