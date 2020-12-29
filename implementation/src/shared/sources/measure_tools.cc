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
#include "measure_tools.h"
#include <ctime>
#include <chrono>
#include <thread>

void
GetTimeStamp (std::ofstream& fout)
{
  time_t t = time(0);
  struct tm* now = localtime( & t );
  fout << now->tm_hour << ' '
       << now->tm_min << ' '
       << now->tm_sec << ' ';
       //<< (now->tm_year + 1900) << '-'
       //<< (now->tm_mon + 1) << '-'
       //<<  now->tm_mday << '-'s
}

void
SleepForOneSec (void)
{
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

void
SleepForOneMilliSec (void)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}