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
#ifndef CodedBulk_APPLICATION_H
#define CodedBulk_APPLICATION_H

#include "address.h"
#include "socket.h"
#include "simple-ref-count.h"
#include "ptr.h"

#include <mutex>
#include <fstream>

class CodedBulkApplication : public SimpleRefCount<CodedBulkApplication> 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  CodedBulkApplication ();
  virtual ~CodedBulkApplication ();

  void SetApplicationID (uint16_t application_id);
  uint16_t GetApplicationID (void) const;

  void SetPriority (uint8_t priority);
  uint8_t GetPriority (void);

  uint64_t GetTotBytes (void);

  uint64_t GetMeasuredBytes (void);

  void Measuring(void);

  void SetThroughProxy (bool through_proxy);
  bool GetThroughProxy (void) const;

  void MeasureThroughput (std::ofstream& fout);

protected:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  bool            m_through_proxy;   // does the traffic go through proxy?
  uint16_t        m_application_id;

  uint64_t        m_totBytes;      //!< Total bytes received
  uint64_t        m_prevTotBytes;    // last recorded bytes
  uint64_t        m_bytes_interval;   // the bytes received within the interval
  uint8_t         m_priority;

  std::mutex      m_totBytes_lock;
};

#endif /* CodedBulk_APPLICATION_H */

