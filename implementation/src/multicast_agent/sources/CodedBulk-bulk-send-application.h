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

#ifndef CodedBulk_BULK_SEND_APPPLICATION_H
#define CodedBulk_BULK_SEND_APPPLICATION_H

#include "CodedBulk-multicast-sender.h"

#include <condition_variable>
#include <thread>

class CodedBulkBulkSendApplication : public CodedBulkMulticastSender {
public:
  CodedBulkBulkSendApplication();
  virtual ~CodedBulkBulkSendApplication();

  void InteractiveInput (uint64_t bytes);

  void StartInteractiveInput (const char* filename);
  void InteractiveInputGenerator (const char* filename);

protected:
  int SendData (CodedBulkMulticastSenderMulticastPath* path);

  virtual void PathReady(CodedBulkMulticastSenderMulticastPath* path);

  uint32_t        m_sendSize;     //!< Size of data to send each time

//  std::list<Ptr<CodedBulkMulticastSenderMulticastPath> > m_ready_paths;

  std::mutex               m_interactive_lock;
  std::condition_variable  m_interactive_condition;

  std::thread              m_interactive_thread;
};

#endif /* CodedBulk_BULK_SEND_APPPLICATION_H */
