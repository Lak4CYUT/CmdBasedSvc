/*
 * ControllerV1.cpp
 *
 *  Created on: 2014年10月8日
 *      Author: lak
 */

#include <string>
#include <queue>
#include <pthread.h>
#include <semaphore.h>
#include <cutils/sockets.h>
#include <sys/un.h>

#include "../../CmdBasedSvc/inc/Interface.h"
#include "../inc/CmdServerV1.h"

#define LOG_NDEBUG 0
#define LOG_TAG "ControllerV1"
#include <cutils/log.h>

//#define DEBUG

namespace CmdBasedSvc
{
    using namespace std;
    extern string dumpBuffer(const char* buf, int len);

    static const std::string g_ctrl_socket_name = string("hipad_comm_def");
    static const int g_max_clients = 30;

    ServiceSerV1::ServerSocketAction::ServerSocketAction(SocketNode* n) :
            SocketAction(n)
    {}

    ServiceSerV1::ServerSocketAction::~ServerSocketAction()
    {}

    void ServiceSerV1::ServerSocketAction::run(void *data)
    {
        int new_socket;
        struct sockaddr_un peeraddr;
        socklen_t socklen = sizeof(peeraddr);
        if ((new_socket = accept(mNode->fd, (sockaddr *) &peeraddr, &socklen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //inform user of socket number - used in send and receive commands
        ALOGV("Host connection , socket fd(%d)\n", new_socket);

        SocketNode* node = new SocketNode();
        node->fd = new_socket;
        node->action = new ClientSocketAction(node);
        node->data = data;
        mNode->last()->add(node);
    }

    ServiceSerV1::ClientSocketAction::ClientSocketAction(SocketNode* n) :
            SocketAction(n)
    {
        cmdParser.setListener(this);
    }

    ServiceSerV1::ClientSocketAction::~ClientSocketAction()
    {
    }

    void ServiceSerV1::ClientSocketAction::onCommandReceiver(Command* cmd)
    {
        // Finally.. we get the command..
        // now, put it into message queue let server to handle it.
        ServiceSerV1* ser = (ServiceSerV1*)mNode->data;
        // Assign source fd for response.
        CommandPackage* cmdPkg = new CommandPackage(cmd, mNode->fd);
        ser->putCommand(cmdPkg);
    }

    void ServiceSerV1::ClientSocketAction::run(void *data)
    {
        char buffer[1024];
        int len = read(mNode->fd, buffer, 1024);
        if (len > 0)
        {
            // Command parser.
            cmdParser.recv(buffer, len);
        }
        else if (len == 0)
        {
            // Exit
            struct sockaddr address;
            socklen_t addrlen = sizeof(address);
            getpeername(mNode->fd, (struct sockaddr*) &address,
                    (socklen_t*) &addrlen);
            ALOGV("Host disconnected (fd=%d)\n", mNode->fd);

            close(mNode->fd);
            mNode->remove();

            // Release self.
            delete mNode->action;
        }
    }

    /*static*/ void* ServiceSerV1::_runServer(void* parm)
    {
        ServiceSerV1* pThis = (ServiceSerV1*) parm;

        SocketNode* firSocNode = new SocketNode;
        firSocNode->action = new ServerSocketAction(firSocNode);
        firSocNode->data = pThis;

        //set of socket descriptors
        fd_set readfds;
        firSocNode->fd = pThis->_socketFd;
        int max_sd = firSocNode->fd;
        int activity;
        timeval timeout;
        //timeout.tv_sec = ;
        while (!pThis->isStopped())
        {
            //clear the socket set
            FD_ZERO(&readfds);

            //add child sockets to set
            SocketNode* tmp = firSocNode;
            while (tmp != NULL)
            {
                //socket descriptor
                int sd = tmp->fd;

                //if valid socket descriptor then add to read list
                if (sd > 0)
                    FD_SET(sd, &readfds);

                //highest file descriptor number, need it for the select function
                if (sd > max_sd)
                    max_sd = sd;

                tmp = tmp->next();

                if (tmp == firSocNode)
                    break;
            }

            //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
            activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

            if ((activity < 0) && (errno != EINTR))
            {
                ALOGE("select error\n");
                tmp = firSocNode;
                while (tmp != NULL)
                {
                    if (FD_ISSET(tmp->fd, &readfds))
                    {
                        ALOGE("error fd(%d)\n", tmp->fd);
                    }
                    tmp = tmp->next();
                }
            }

            //If something happened on the master socket , then its an incoming connection
            tmp = firSocNode;
            while (tmp != NULL)
            {
                if (FD_ISSET(tmp->fd, &readfds))
                {
                    tmp->action->run(pThis);
                }
                tmp = tmp->next();
            }
        }

        // TODO
        // Release everything....
        return 0;
    }

    /*static*/ void* ServiceSerV1::_cmdDispatcher(void* parm)
    {
        ServiceSerV1* pThis = (ServiceSerV1*) parm;
        while (!pThis->isStopped())
        {
#ifdef DEBUG
            fprintf(stderr, "_cmdDispatcher waiting...\n");
#endif
            CommandPackage *cmdPkg = pThis->_msgQueue.waitAndPop();
#ifdef DEBUG
            fprintf(stderr, "_cmdDispatcher receive content=%s\n", cmdPkg->cmd->content);
#endif
            ALOGW("_cmdDispatcher receive content=%s", cmdPkg->cmd->content);

            if (cmdPkg != NULL && pThis->_cmdListener != NULL)
            {
                pThis->_cmdListener->onReceiver(cmdPkg);
            }
        }
        return 0;
    }

    ServiceSerV1::ServiceSerV1() :
        _serverThreadId(0)
    {}

    ServiceSerV1::~ServiceSerV1()
    {}

    bool ServiceSerV1::initServer(const void * attr)
    {
        if (attr != NULL)
            _localSocketName = string((char*)attr);
        else
            _localSocketName = g_ctrl_socket_name;

        int ret = socket_local_server(_localSocketName.c_str(),
        ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

        if (ret < 0)
        {
            ALOGE("Unable to bind socket errno:%d", errno);
            exit(-1);
        }
        _socketFd = ret;

        // TODO
        // Future work. Replace socket_local_server with android_get_control_socket
        /*
         s_fdListen = android_get_control_socket(RIL_getRilSocketName());
         if (s_fdListen < 0) {
         RLOGE("Failed to get socket %s",RIL_getRilSocketName());
         exit(-1);
         }

         ret = listen(s_fdListen, 4);

         if (ret < 0) {
         RLOGE("Failed to listen on control socket '%d': %s",
         s_fdListen, strerror(errno));
         exit(-1);
         }
         */
        return true;
    }

    bool ServiceSerV1::startServer()
    {
        if (!_isStopped)
            return false;

        _isStopped = false;

        pthread_t tid;
        int err = pthread_create(&tid, NULL, &_runServer, this);
        ALOGI("Start server host thread.");
        if (err)
        {
            ALOGE("startServer _runServer pthread_create fail. errcode = %d", err);
            return false;
        }
        _serverThreadId = tid;
        ALOGI("Server host thread ready. tid= %lu", tid);

        ALOGI("Start command dispatcher thread.");
        err = pthread_create(&tid, NULL, &_cmdDispatcher, this);
        if (err)
        {
            ALOGE("startServer _cmdDispatcher pthread_create fail. errcode = %d", err);
            return false;
        }
        ALOGI("Command dispatcher thread ready.");
        return true;
    }

    bool ServiceSerV1::stopServer()
    {
        // TODO
        _isStopped = true;

        // Stop socket and cause _runServer thread end.
        close(_socketFd);
        _msgQueue.kick();
        return true;
    }

    void ServiceSerV1::sendResp(CmdBasedSvc::CommandPackage* cmdPkg)
    {
        int fd = (int)cmdPkg->source;
        if (fd < 0)
            return;

        char* buf = NULL;
        int len = CmdParser::parserCmd(cmdPkg->cmd, &buf);

#ifdef DEBUG
        string str = dumpBuffer(buf, len);
        fprintf(stderr, "%s\n", str.c_str());
        fprintf(stderr, "Resp len: %d\n", len);
#endif

        int res = write(fd, buf, len);

        if (res != len)
        {
            ALOGE("sendResp fail, len=%d, res=%d, errno=%d", len, res, errno);
        }

        delete [] buf;
    }

    void ServiceSerV1::regCmdListener(CommandPackageListener* listener)
    {
        _cmdListener = listener;
    }

    void ServiceSerV1::putCommand(CommandPackage* cmdPkg)
    {
        _msgQueue.push(cmdPkg);
    }

    int ServiceSerV1::joinServer()
    {
        return pthread_join(_serverThreadId, NULL);
    }

// *******************   Client part  ********************** //
    ControllerV1::ControllerV1()
    {
        cmdParser.setListener(this);
    }

    bool ControllerV1::initClient(const void * attr)
    {
        if (attr != NULL)
            _localSocketName = string((char*)attr);
        else
            _localSocketName = g_ctrl_socket_name;

        _socketFd = socket_local_client(_localSocketName.c_str(),
               ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
        if (_socketFd < 0)
        {
            ALOGE("ControllerCntV1 connect fail, cannot create socket.");
            return false;
        }

        return true;
    }

    /*static*/ void* ControllerV1::_respReceiver(void* parm)
    {
        ControllerV1* pThis = (ControllerV1*)parm;
        fd_set readfds;
        int activity;
        timeval timeout;
        //timeout.tv_sec = ;
        while (pThis->isConnected())
        {
            //clear the socket set
            FD_ZERO(&readfds);
            FD_SET(pThis->_socketFd, &readfds);
            activity = select(pThis->_socketFd + 1, &readfds, NULL, NULL, NULL);
            if ((activity < 0) && (errno != EINTR))
            {
                ALOGE("Controller select error");
                pThis->disconnect();
                break;
            }

            if (!FD_ISSET(pThis->_socketFd, &readfds))
                 continue;

            char buffer[1024];
            int len = read(pThis->_socketFd, buffer, 1024);
            if (len > 0)
            {
                // Command parser.
                pThis->cmdParser.recv(buffer, len);
            }
            else if (len == 0)
            {
                // Exit
                struct sockaddr address;
                socklen_t addrlen = sizeof(address);
                getpeername(pThis->_socketFd, (struct sockaddr*) &address,
                        (socklen_t*) &addrlen);
                ALOGV("Host disconnected (fd=%d)\n", pThis->_socketFd);
                pThis->disconnect();
                break;
            }
        }
        return 0;
    }

    bool ControllerV1::connect()
    {
        // Create a receive thread to receive the response.
        if (isConnected())
            return false;

        connected = true;

        pthread_t tid;
        int err = pthread_create(&tid, NULL, &_respReceiver, this);
        ALOGI("Start server host thread.");
        if (err)
        {
            ALOGE("connect _respReceiver pthread_create fail. errcode = %d", err);
            return false;
        }
        ALOGI("RespReceiver thread ready. tid= %lu", tid);
        return true;
    }

    bool ControllerV1::disconnect()
    {
        if (!isConnected())
            return true;

        connected = false;
        if (_socketFd < 0)
        {
            return true;
        }

        // TODO
        // Maybe send a command??
        close(_socketFd);
        return true;
    }

    void ControllerV1::sendCmd(CmdBasedSvc::Command* cmd)
    {
        if (_socketFd < 0)
            return;

        char* buf = NULL;
        int len = CmdParser::parserCmd(cmd, &buf);

#ifdef DEBUG
        string str = dumpBuffer(buf, len);
        fprintf(stderr, "%s\n", str.c_str());
#endif

        int res = write(_socketFd, buf, len);

        if (res != len)
        {
            ALOGE("sendCmd fail, len=%d, res=%d, errno=%d", len, res, errno);
        }

        delete [] buf;
    }

    void ControllerV1::regRespListener(CommandPackageListener* l)
    {
        listener = l;
    }

    void ControllerV1::onCommandReceiver(Command* cmd)
    {
        CommandPackage* cmdPkg = new CommandPackage(cmd, 0);
        listener->onReceiver(cmdPkg);
    }
}
