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
	CommandServer *ser = new ServiceSerV1();
    ser->initServer("hicatd");
    ser->startServer();

    int i;
    std::cin >> i;
    return 1;
}


