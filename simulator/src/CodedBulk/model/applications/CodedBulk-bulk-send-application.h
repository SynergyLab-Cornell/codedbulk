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

#include "ns3/CodedBulk-multicast-sender.h"
#include "ns3/nstime.h"

namespace ns3 {

class CodedBulkBulkSendApplication : public CodedBulkMulticastSender {
public:
  static TypeId GetTypeId (void);

  CodedBulkBulkSendApplication();

  // call before simulation
  void InteractiveInputAt (Time time, uint64_t bytes);
  void InteractiveInput (uint64_t bytes);

protected:
  int SendData (Ptr<CodedBulkMulticastSenderMulticastPath> path);

  virtual void PathReady(Ptr<CodedBulkMulticastSenderMulticastPath> path);

  uint32_t        m_sendSize;     //!< Size of data to send each time

  std::list<Ptr<CodedBulkMulticastSenderMulticastPath> > m_ready_paths;
};

} // namespace ns3

#endif /* CodedBulk_BULK_SEND_APPPLICATION_H */
