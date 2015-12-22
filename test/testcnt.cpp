/*
 * test.cpp
 *
 *  Created on: 2014年10月13日
 *      Author: lak
 */
#include <unistd.h>
#include <iostream>

#include "../inc/CmdServerV1.h"

using namespace CmdBasedSvc;

int main()
{
    ControllerV1 *cnt = new ControllerV1();
    cnt->initClient("hicatd");
    cnt->connect();

    while (true)
    {

        std::string i;
        std::cin >> i;

        Command cmd;
        cmd.content = new char[i.size() + 1];
        strcpy(cmd.content, i.c_str());
        cmd.content[i.size()] = '\0';
        //cmd.content = i.c_str();
        cnt->sendCmd(&cmd);
    }

    return 1;
}


