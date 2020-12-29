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
#include "CodedBulk-application.h"
#include "measure_tools.h"

CodedBulkApplication::CodedBulkApplication () :
  m_through_proxy(false),
  m_totBytes(0),
  m_prevTotBytes(0),
  m_bytes_interval(0)
{
}

CodedBulkApplication::~CodedBulkApplication()
{
}

void
CodedBulkApplication::SetApplicationID (uint16_t application_id)
{
  m_application_id = application_id;
}

uint16_t
CodedBulkApplication::GetApplicationID (void) const
{
  return m_application_id;
}

void
CodedBulkApplication::SetPriority (uint8_t priority)
{
  m_priority = priority;
}

uint8_t
CodedBulkApplication::GetPriority (void)
{
  return m_priority;
}

uint64_t
CodedBulkApplication::GetTotBytes (void)
{
  return m_totBytes;
}

uint64_t
CodedBulkApplication::GetMeasuredBytes (void)
{
  // return the measured bytes within the previous interval
  return m_bytes_interval;
}

void
CodedBulkApplication::Measuring(void)
{
  m_totBytes_lock.lock();
  m_bytes_interval = m_totBytes - m_prevTotBytes;
  m_prevTotBytes = m_totBytes;
  m_totBytes_lock.unlock();
}

void
CodedBulkApplication::StartApplication (void)
{}

void
CodedBulkApplication::StopApplication (void)
{}

void
CodedBulkApplication::SetThroughProxy (bool through_proxy)
{
  m_through_proxy = through_proxy;
}

bool
CodedBulkApplication::GetThroughProxy (void) const
{
  return m_through_proxy;
}

void
CodedBulkApplication::MeasureThroughput (std::ofstream& fout)
{
  Measuring ();
  GetTimeStamp (fout);
  m_totBytes_lock.lock();
  fout << m_bytes_interval << std::endl;
  m_totBytes_lock.unlock();
}