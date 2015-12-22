/*
 * ControllerV1.h
 *
 *  Created on: 2014年10月8日
 *      Author: lak
 */
#include <string>
#include <queue>
#include <semaphore.h>

#include "../../CmdBasedSvc/inc/Comm.h"
#include "../../CmdBasedSvc/inc/Interface.h"

namespace CmdBasedSvc
{
using namespace std;

    class ServiceSerV1 : public CommandServer
    {
    private:
        class ClientSocketAction;

        string _localSocketName;
        int _socketFd;
        MessageQueue _msgQueue;
        CommandPackageListener* _cmdListener;
        pthread_t _serverThreadId;

        class ServerSocketAction : public SocketAction
        {
        public:
            ServerSocketAction(SocketNode* n);
            ~ServerSocketAction();
            virtual void run(void *data);
        };

        class ClientSocketAction : public SocketAction, public CmdListener
        {
        private:
            CmdParser cmdParser;
        public:
            ClientSocketAction(SocketNode* n);
            ~ClientSocketAction();

            void onCommandReceiver(Command* cmd);
            virtual void run(void *data);
        };

        static void* _runServer(void* parm);
        static void* _cmdDispatcher(void* parm);
    public:
        // Constructor
        ServiceSerV1();
        virtual ~ServiceSerV1();

        // attr will be a char* string, defined the local socket name.
        virtual bool initServer(const void* attr);
        virtual bool startServer();
        virtual bool stopServer();
        virtual void sendResp(CmdBasedSvc::CommandPackage* cmdPkg);
        virtual void regCmdListener(CommandPackageListener* listener);
        void putCommand(CommandPackage* cmd);
        // TODO
        // Workaround, it should be replace with a status listener.
        int joinServer();
    };

    class ControllerV1 : public CommandClient, public CmdListener
    {
    private:
        string _localSocketName;
        int _socketFd;
        CommandPackageListener* listener;
        CmdParser cmdParser;

        static void* _respReceiver(void* parm);
    public:
        ControllerV1();
        virtual ~ControllerV1() {};
        // attr will be a char* string, defined the local socket name.
        virtual bool initClient(const void * attr);
        virtual bool connect();
        virtual bool disconnect();
        virtual void sendCmd(CmdBasedSvc::Command* cmd);
        virtual void regRespListener(CommandPackageListener* listener);
        virtual void onCommandReceiver(Command* cmd);
    };
}
