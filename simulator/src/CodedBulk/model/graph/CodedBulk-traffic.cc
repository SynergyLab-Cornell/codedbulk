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
#include "CodedBulk-traffic.h"

namespace ns3 {

CodedBulkTraffic::CodedBulkTraffic (int src_id) :
  _src_id(src_id),
  _is_CodedBulk_traffic(false),
  _is_unicast(false),
  _priority(BestEffort)
{
    _dst_id.clear();
    _interactive_workload.clear();
    clearCodedBulkMultipathSets();
}

CodedBulkTraffic::~CodedBulkTraffic() {
    clearCodedBulkMultipathSets();
}

void
CodedBulkTraffic::operator = (const CodedBulkTraffic& traffic) {
    _id              = traffic._id       ;
    _src_id          = traffic._src_id   ;
    _dst_id          = traffic._dst_id   ;
    _path_sets       = traffic._path_sets;
    _is_CodedBulk_traffic   = traffic._is_CodedBulk_traffic;
    _is_unicast      = traffic._is_unicast;
    _is_Steiner_tree = traffic._is_Steiner_tree;
    _priority        = traffic._priority;
    _interactive_workload = traffic._interactive_workload;
}

bool
CodedBulkTraffic::addDst (int dst_id) {
    // do not erase the current path sets
    if( _path_sets.find(dst_id) == _path_sets.end() ) {
        _path_sets[dst_id] = Create<CodedBulkMultipathSet> ();
        _dst_id.push_back (dst_id);
        return true;
    } else {
        // the destination has already existed
        return false;
    }
}

uint32_t
CodedBulkTraffic::GetNumDst (void) {
    return _dst_id.size();
}

void
CodedBulkTraffic::IsCodedBulk (bool is_codedbulk)
{
    _is_codedbulk = is_codedbulk;
}

void
CodedBulkTraffic::ApplyCodedBulk (bool is_CodedBulk_traffic)
{
    _is_CodedBulk_traffic = is_CodedBulk_traffic;
}

void
CodedBulkTraffic::SetUnicast (bool is_unicast)
{
    _is_unicast = is_unicast;
}

void
CodedBulkTraffic::SetSteinerTree (bool is_Steiner_tree)
{
    _is_Steiner_tree = is_Steiner_tree;
    //if (is_Steiner_tree) {
        // using proxies
    //    _is_CodedBulk_traffic = true;
    //}
}

void
CodedBulkTraffic::SetPriority (CodedBulkTrafficPriority priority)
{
    switch (priority) {
        case Bulk:
        case InteractiveBulk:
        case Interactive:
            _priority = priority;
            break;
        default:
            _priority = BestEffort;
            break;
    }
}

void
CodedBulkTraffic::addWorkload (double time_ms, uint64_t size)
{
    CodedBulkTrafficLoad load;
    load._time_ms = time_ms;
    load._size    = size;
    _interactive_workload.push_back(load);
}

Ptr<CodedBulkMultipathSet>
CodedBulkTraffic::getCodedBulkMultipathSet (int dst_id) {
    if( _path_sets.find(dst_id) == _path_sets.end() ) {
        return nullptr;
    }
    return _path_sets[dst_id];
}

void
CodedBulkTraffic::clearCodedBulkMultipathSets() {
    for(std::map<int, Ptr<CodedBulkMultipathSet>>::iterator it = _path_sets.begin(); it != _path_sets.end(); ++it) {
        it->second->clearAllPaths();
    }
}

void
CodedBulkTraffic::listAllPathSets(std::ostream& os) {
    for(std::map<int, Ptr<CodedBulkMultipathSet>>::iterator it = _path_sets.begin(); it != _path_sets.end(); ++it) {
        it->second->listPath(os);
        os << std::endl;
    }
}

uint64_t
CodedBulkTraffic::GetTotalMeasuredBytes (void) {
    uint64_t total = 0;
    for(std::list<Ptr<CodedBulkReceiver> >::iterator it = _receivers.begin(); it != _receivers.end(); ++it) {
        total += (*it)->GetMeasuredBytes();
    }
    return total;
}

int CodedBulkTrafficManager::m_traffic_id = 0;

CodedBulkTrafficManager::CodedBulkTrafficManager () {
    clearAllCodedBulkTraffic();
}

CodedBulkTrafficManager::~CodedBulkTrafficManager () {
    clearAllCodedBulkTraffic();
}

void
CodedBulkTrafficManager::clearAllCodedBulkTraffic () {
    m_traffic_not_generated = true;
    m_traffic.clear();
}

void
CodedBulkTrafficManager::listAllCodedBulkTraffic (std::ostream& os) {
    for(std::list<Ptr<CodedBulkTraffic> >::iterator
        it  = m_traffic.begin();
        it != m_traffic.end();
        ++it
    ) {
        os << "traffic " << (*it)->_id << " from : " << (*it)->_src_id << std::endl;
        (*it)->listAllPathSets(os);
    }
}

Ptr<CodedBulkTraffic>
CodedBulkTrafficManager::addCodedBulkTraffic (int src_id) {
    Ptr<CodedBulkTraffic> traffic = Create<CodedBulkTraffic> (src_id);
    traffic->_id = m_traffic_id++;
    m_traffic.push_back(traffic);
    return traffic;
}

double
CodedBulkTrafficManager::GetAverageThroughput (void) const {
    return GetTotalThroughput()/m_traffic.size();
}

double
CodedBulkTrafficManager::GetTotalThroughput (void) const {
    double total_throughput = 0.0;
    for(std::list<Ptr<CodedBulkTraffic> >::const_iterator
        it  = m_traffic.begin();
        it != m_traffic.end();
        ++it
    ) {
        total_throughput += (double) (*it)->GetTotalMeasuredBytes () / (*it)->GetNumDst();
    }

    return total_throughput;
}

double
CodedBulkTrafficManager::GetTotalBulkThroughput (void) const {
    double total_throughput = 0.0;
    for(std::list<Ptr<CodedBulkTraffic> >::const_iterator
        it  = m_traffic.begin();
        it != m_traffic.end();
        ++it
    ) {
        if ((*it)->_priority == CodedBulkTraffic::Bulk) {
            total_throughput += (double) (*it)->GetTotalMeasuredBytes () / (*it)->GetNumDst();
        }
    }
    return total_throughput;
}

double
CodedBulkTrafficManager::GetTotalInteractiveThroughput (void) const {
    double total_throughput = 0.0;
    for(std::list<Ptr<CodedBulkTraffic> >::const_iterator
        it  = m_traffic.begin();
        it != m_traffic.end();
        ++it
    ) {
        if ((*it)->_priority == CodedBulkTraffic::Interactive) {
            total_throughput += (double) (*it)->GetTotalMeasuredBytes () / (*it)->GetNumDst();
        }
    }
    return total_throughput;
}

void
CodedBulkTrafficManager::SetTrafficNotGenerated (bool traffic_not_generated) {
    m_traffic_not_generated = traffic_not_generated;
}

bool
CodedBulkTrafficManager::GetTrafficNotGenerated (void) {
    return m_traffic_not_generated;
}

}