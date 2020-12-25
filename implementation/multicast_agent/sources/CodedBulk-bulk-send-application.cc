/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Shih-Hao Tseng <st688@cornell.edu>
//
#include "test_tools.h"
#include "measure_tools.h"
#include "packet.h"

#include <fstream>
#include <chrono>

#include "CodedBulk-bulk-send-application.h"

CodedBulkBulkSendApplication::CodedBulkBulkSendApplication() : 
  m_sendSize(1000)
{
  //m_ready_paths.clear();
}

CodedBulkBulkSendApplication::~CodedBulkBulkSendApplication()
{
  m_interactive_lock.lock();
  if (m_interactive_thread.joinable())
    m_interactive_thread.join();
  m_interactive_lock.unlock();
}

void
CodedBulkBulkSendApplication::InteractiveInput (uint64_t bytes)
{
  if(bytes > 0) {
    m_maxBytes += bytes;
    m_interactive_condition.notify_all();
  }
}

void
CodedBulkBulkSendApplication::StartInteractiveInput (const char* filename)
{
  if(GetPriority() != 6) {
    // not an interactive traffic
    return;
  }
  m_interactive_lock.lock();
  m_interactive_thread = std::thread (&CodedBulkBulkSendApplication::InteractiveInputGenerator, this, filename);
  m_interactive_lock.unlock();
}

void
CodedBulkBulkSendApplication::InteractiveInputGenerator (const char* filename)
{
  std::ifstream fin (filename);

  if(!fin) {
    return;
  }

  long long int prev_time_ms = 0;
  long long int int_time_ms;
  uint64_t int_flow_size;

  while (fin >> int_time_ms >> int_flow_size) {
    std::this_thread::sleep_for(std::chrono::milliseconds(int_time_ms - prev_time_ms));
    InteractiveInput (int_flow_size);
    prev_time_ms = int_time_ms;
  }
}

int
CodedBulkBulkSendApplication::SendData (CodedBulkMulticastSenderMulticastPath* path)
{
    if ((GetPriority() < 6) || (m_totBytes < m_maxBytes))
    { // Time to send more

      // uint64_t to allow the comparison later.
      // the result is in a uint32_t range anyway, because
      // m_sendSize is uint32_t.
      uint64_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (toSend, m_maxBytes - m_totBytes);
        }
      path->m_packet.initialize();
      // for testing
      TEST_REGION(
        std::stringstream ss;
        ss << "st688 " << m_totBytes << "\n";
        memcpy(path->m_packet.GetBuf(), ss.str().c_str(), ss.str().size());
        path->m_packet.MoveWriteHead (ss.str().size());
        //memcpy(m_packet_buf, "st688\n", 6);
      );
      path->m_packet.SetSize(toSend);
      SendPacket(path, &path->m_packet);
      TEST_REGION(
        packet.Print(std::cout);
      );
      return 0;
    }
  else
    {
      std::unique_lock<std::mutex> lock(m_interactive_lock);
      m_interactive_condition.wait(lock);
    }

  return -1;
}

void
CodedBulkBulkSendApplication::PathReady(CodedBulkMulticastSenderMulticastPath* path)
{
  SendData (path);
}

