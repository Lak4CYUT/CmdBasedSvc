/*
 * Comm.cpp
 *
 *  Created on: 2014年10月14日
 *      Author: lak
 */
#include "../../CmdBasedSvc/inc/Comm.h"

#define LOG_TAG "Comm"
#include <stdio.h>
#include <string>
#include <cutils/log.h>

namespace CmdBasedSvc
{
    using namespace std;

    string dumpBuffer(const char* buf, int len);

    SocketNode::SocketNode() :
            _prev(NULL), _next(NULL)
    {}

    SocketNode::~SocketNode()
    {}


    SocketNode* SocketNode::next()
    {
        return _next;
    }

    SocketNode* SocketNode::last()
    {
        SocketNode* tmp;
        tmp = this;
        while (tmp->_next != NULL)
        {
            tmp = tmp->_next;

            // Avoid a circular search.
            if (tmp == this)
                break;
        }
        return tmp;
    }

    void SocketNode::add(SocketNode* node)
    {
        if (node == this)
            return;

        node->_prev = this;
        if (_next != NULL)
        {
            SocketNode* tmp = _next;
            SocketNode* last = node->last();
            last->_next = tmp;
            tmp->_prev = last;
        }
        _next = node;
    }

    SocketNode* SocketNode::remove()
    {
        if (_next != NULL && _prev != NULL)
        {
            _next->_prev = _prev;
            _prev->_next = _next;

            _next = NULL;
            _prev = NULL;
        }
        else if (_next != NULL)
        {
            _next->_prev = NULL;
            _next = NULL;
        }
        else if (_prev != NULL)
        {
            _prev->_next = NULL;
            _prev = NULL;
        }
        return this;
    }

    SocketAction::SocketAction(SocketNode* n)
    {
        mNode = n;
    }

    MessageQueue::MessageQueue()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mtx, &attr);

        sem_init(&dataWait, 0, 0);
    }

    void MessageQueue::push(CommandPackage* msg)
    {
        pthread_mutex_lock(&mtx);
        cmdQ.push(msg);
        pthread_mutex_unlock(&mtx);
        sem_post(&dataWait);
    }

    bool MessageQueue::empty()
    {
        pthread_mutex_lock(&mtx);
        bool val = cmdQ.empty();
        pthread_mutex_unlock(&mtx);
        return val;
    }

    bool MessageQueue::pop(CommandPackage** popedMsg)
    {
        pthread_mutex_lock(&mtx);
        if (empty())
        {
            pthread_mutex_unlock(&mtx);
            return false;
        }

        *popedMsg = cmdQ.front();
        cmdQ.pop();

        pthread_mutex_unlock(&mtx);
        return true;
    }

    CommandPackage* MessageQueue::waitAndPop()
    {
        CommandPackage* msg;
        while (empty())
        {
            sem_wait(&dataWait);
        }

        if (!pop(&msg))
        {
            //TODO
            // ERROR
            return NULL;
        }

        return msg;
    }

    void MessageQueue::kick()
    {
        sem_post(&dataWait);
    }

    CmdParser::CmdParser() :
            cmdBuf(NULL), cmdSize(0), cmdOffset(0)
    {
        sem_init(&dataWait, 0, 0);
    }

    CmdParser::~CmdParser()
    {
        delete[] cmdBuf;
    }

    void CmdParser::setListener(CmdListener* l)
    {
        listener = l;
    }

    /*static*/ int CmdParser::parserCmd(Command *cmd, char** buf)
    {
        //int contentSize = strlen(cmd->content) + 1;
        int contentSize = strlen(cmd->content);
        int headerSize = HEADER_SIZE;
        int len = headerSize + contentSize;

        *buf = new char[len];
        char* locbuf = *buf;
        // Copy header
        memcpy(locbuf, &contentSize, headerSize);
        // Copy content
        //memcpy(&locbuf[headerSize], cmd->content.c_str(), strlen(cmd->content.c_str()));
        //strcpy(&locbuf[headerSize], cmd->content);
        memcpy(&locbuf[headerSize], cmd->content, contentSize);

#ifdef DEBUG
        fprintf(stderr, "contentSize: %d\n", contentSize);
        string logstr = dumpBuffer(locbuf, len);
        fprintf(stderr, "%s\n", logstr.c_str());
#endif

        return len;
    }

    bool CmdParser::inflateCmd(char* buf, int len)
    {
#ifdef DEBUG
        fprintf(stderr, "inflateCmd() cmdOffset %d\n", cmdOffset);
#endif
        memcpy(&cmdBuf[cmdOffset], buf, len);
        cmdOffset += len;
        if (cmdOffset == cmdSize)
        {
            // Receive a Command complete.
            // Create command object and put it into message queue.

            // Repair the C style null-terminator.
            char* strBuf = new char[cmdSize + 1];
            memcpy(strBuf, cmdBuf, cmdSize);
            strBuf[cmdSize] = '\0';

            string content = string(strBuf);

#ifdef DEBUG
            {
                string str = dumpBuffer(cmdBuf, cmdSize);
                fprintf(stderr, "%s\n", str.c_str());
                fprintf(stderr, "inflateCmd() content: %s\n", content.c_str());
            }
#endif

            if (listener != NULL)
            {
                Command* cmd = new Command();
                cmd->content = new char[content.size() + 1];
                strcpy(cmd->content, content.c_str());
                cmd->content[content.size()] = '\0';
                //cmd->content = str.c_str();
                listener->onCommandReceiver(cmd);
            }

            cmdSize = 0;
            cmdOffset = 0;
            delete[] cmdBuf;
            cmdBuf = NULL;

            return true;
        }
        else if (cmdOffset < cmdSize)
        {
            // Don't complete
            return false;
        }
        else
        {
            // TODO
            // ERROR
            ALOGE("Command size is not match!!!!!!!!!");
            return false;
        }
    }

    void CmdParser::recv(char* buf, int bufSize)
    {
        int bufOffset = 0;

        while (1)
        {
            if (bufOffset == bufSize)
                break;

            // Read the command length.
            if (cmdBuf == NULL)
            {
                memcpy(&cmdSize, &buf[bufOffset], HEADER_SIZE);

                ALOGV("Parser: CMD length: %d", cmdSize);
#ifdef DEBUG
                fprintf(stderr, "Parser: CMD length: %d\n", cmdSize);
#endif
                bufOffset += HEADER_SIZE;
                // If a command length equal 0,
                // that means this connection finish.
                if (cmdSize == 0)
                {
                    // TODO
                }
                else
                    cmdBuf = new char[cmdSize];
            }

            int len;
            if ((bufSize - bufOffset) >= (cmdSize - cmdOffset))
            {
                // The received buffer is enough to complete a command.
                len = cmdSize - cmdOffset;
                inflateCmd(&buf[bufOffset], len);
            }
            else
            {
                len = bufSize - bufOffset;
                inflateCmd(&buf[bufOffset], len);
            }

            bufOffset += len;
        }
    }

    string dumpBuffer(const char* buf, int len)
    {
        char tmp[10];
        string logstr;
        for (int i=0; i<len; i++)
        {
            snprintf(tmp, sizeof(tmp), "0x%02x", buf[i]);
            logstr += tmp;
            logstr += " ";
        }
        return logstr;
    }
}



