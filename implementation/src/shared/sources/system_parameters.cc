/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Shih-Hao Tseng (st688@cornell.edu)
 */
#include "system_parameters.h"
#include "socket.h"
#include <iostream>
#include <stdio.h>
#include <signal.h>

SystemParameters system_parameters;

void signal_handler(int signum)
{
  fprintf(stderr, "Caught signal %d\n", signum);

  system_parameters.setRunning(false);
  system_parameters.tryJoin();

  exit(signum);
}

SystemParameters::SystemParameters () {
    signal(SIGPIPE,signal_handler);
    //signal(SIGPIPE, SIG_IGN);
    
    _mutex.lock();
    _running = true;
    _listening_sockets.clear();
    _mutex.unlock();
}

SystemParameters::~SystemParameters () {
    setRunning(false);
    tryJoin();
}

void
SystemParameters::setRunning (bool running) {
    _mutex.lock();
    _running = running;
    if (!running) {
        for(auto& socket : _listening_sockets) {
            socket->ShutdownRD();
        }
        _listening_sockets.clear();
    }
    _mutex.unlock();
}

void
SystemParameters::registerListeningSocket(Socket* socket) {
    _mutex.lock();
    _listening_sockets.push_back(socket);
    _mutex.unlock();
}

void
SystemParameters::waitForInput() {
    _wait_for_input = std::thread(&SystemParameters::waitForInputThread,this);
}

void
SystemParameters::tryJoin() {
    if (_wait_for_input.joinable())
        _wait_for_input.join();
}

void
SystemParameters::waitForInputThread() {
    std::cin.get();
    setRunning(false);
}