#ifndef __SERVER_H
#define __SERVER_H

#include "comm.h"
#include "immsbase.h"

class ImmsServer : public SocketServer, virtual public ImmsBase
{
public:
    ImmsServer();
    virtual ~ImmsServer();
    void do_events();
protected:
    virtual void reset_selection() = 0;

    Socket *conn;
    string filter;
};     

#endif
