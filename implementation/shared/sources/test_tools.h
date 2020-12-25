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
#ifndef TEST_TOOLS_H
#define TEST_TOOLS_H

//#define TEST_MODE_ON

#ifdef TEST_MODE_ON

#include <iostream>
#include <sstream>

#define TEST_REGION(x)                         \
  do{                                          \
    x                                          \
  }while(false)
#define TEST_MESSAGE(x)                        \
  do{                                          \
    std::cout << x << std::endl << std::flush; \
  }while(false)
#define TEST_FUNCTION_NAME()                   \
  do{                                          \
    std::cout << __FUNCTION__ << std::endl << std::flush; \
  }while(false)

#else

#define TEST_REGION(x)
#define TEST_MESSAGE(x)
#define TEST_FUNCTION_NAME()

#endif

#endif /* TEST_TOOLS_H */