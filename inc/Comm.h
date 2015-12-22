/*
 * Comm.h
 *
 *  Created on: 2014年10月14日
 *      Author: lak
 */

#ifndef COMMAND_BASED_SERVICE_INC_COMM_H_
#define COMMAND_BASED_SERVICE_INC_COMM_H_

#include <pthread.h>
#include <semaphore.h>
#include <queue>

#include "../../CmdBasedSvc/inc/Interface.h"

namespace CmdBasedSvc
{
using namespace std;
    class SocketAction;

    class SocketNode
    {
    private:
        SocketNode* _prev;
        SocketNode* _next;
    public:
        // OO or func point?? hmm.. func point now...
        SocketAction* action;
        void* data;
        int fd;

        SocketNode();
        ~SocketNode();

        SocketNode* next();
        SocketNode* last();
        void add(SocketNode* node);
        SocketNode* remove();
    };

    class SocketAction
    {
    protected:
        SocketNode* mNode;

    public:
        virtual ~SocketAction() {};

        SocketAction(SocketNode* n);
        virtual void run(void *data)=0;
    };

    class MessageQueue
    {
    private:
        std::queue<CommandPackage*> cmdQ;
        pthread_mutex_t mtx;
        sem_t dataWait;

    public:
        MessageQueue();
        void push(CommandPackage* msg);
        bool empty();
        bool pop(CommandPackage** popedMsg);
        CommandPackage* waitAndPop();

        // it will cause the MessageQueue.waitAndPop return a NULL.
        void kick();
    };

    class CmdParser
    {
    private:
        static const int HEADER_SIZE = sizeof(int);
        char* cmdBuf;
        int cmdSize;
        int cmdOffset;
        CmdListener* listener;
        sem_t dataWait;

    public:
        CmdParser();
        ~CmdParser();
        void setListener(CmdListener* l);
        static int parserCmd(Command *cmd, char** buf);

        bool inflateCmd(char* buf, int len);
        void recv(char* buf, int bufSize);
    };
}

#endif /* COMMAND_BASED_SERVICE_INC_COMM_H_ */
