/*
 * CmdServer.h
 *
 *  Created on: 2014年10月8日
 *      Author: lak
 */

#ifndef COMMAND_BASED_SERVICE_INC_INTERFACE_H_
#define COMMAND_BASED_SERVICE_INC_INTERFACE_H_
#include <string>

namespace CmdBasedSvc
{
    class Command
    {
    public:
        // Suppose is a JSON string.
        // It is a easy and flexible communication protocol.
        // !! string exist a version problem(gunstl v.s stlport),
        // it will invoke some problem.
        //std::string content;
        char* content;
    };

    class CommandPackage
    {
    public:
        Command* cmd;
        // TODO
        // It is not a good design, I will change it when I am free.
        int source;

        CommandPackage(Command* c, int s) : cmd(c), source(s) {};
    };

    class CmdListener
    {
    public:
        virtual ~CmdListener() {};
        virtual void onCommandReceiver(Command* cmd)=0;
    };

    class CommandPackageListener
    {
    public:
        virtual ~CommandPackageListener() {};
        virtual void onReceiver(CommandPackage* cmdPkg)=0;
    };

    class ResponseListener
    {};

    class CommandServer
    {
    protected:
        bool _isStopped ;
    public:
        // Constructor
        CommandServer() : _isStopped(true) {};
        virtual ~CommandServer() {};

        virtual bool initServer(const void *)=0;
        virtual bool startServer()=0;
        virtual bool stopServer()=0;
        inline bool isStopped() const
        { return _isStopped; };

        // TODO
        // Server should not need to "send" a command initiative but
        // it need to "response" a command.
        virtual void sendResp(CmdBasedSvc::CommandPackage* cmdPkg)=0;
        virtual void regCmdListener(CommandPackageListener* listener)=0;
        // TODO
        //virtual void regStatusListener(StatusListener* listener)=0;
    };

    class CommandClient
    {
    protected:
        bool connected ;
    public:
        CommandClient() : connected(false) {};
        virtual ~CommandClient() {};

        virtual bool initClient(const void *)=0;
        virtual bool connect()=0;
        virtual bool disconnect()=0;

        inline bool isConnected() const
        { return connected; };

        virtual void sendCmd(CmdBasedSvc::Command* cmd)=0;
        virtual void regRespListener(CommandPackageListener* listener)=0;
    };
}

#endif /* COMMAND_BASED_SERVICE_INC_INTERFACE_H_ */
