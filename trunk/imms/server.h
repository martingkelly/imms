#ifndef __SERVER_H
#define __SERVER_H

#include "comm.h"
#include "immsbase.h"

class ImmsServer : public SocketServer, virtual public ImmsBase
{
public:
    ImmsServer();
    ~ImmsServer();
    void do_events();
protected:
    Socket *conn;
    string filter;
};     

#endif
