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

CodedBulkTraffic::CodedBulkTraffic (int src_id) :
  _src_id(src_id),
  _is_CodedBulk_traffic(false),
  _is_unicast(false),
  _priority(BestEffort)
{
    _dst_id.clear();
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
    //    // using proxies
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
    m_traffic_id = 0;
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

void
CodedBulkTrafficManager::listTrafficSummary (std::ostream& os) {
    // using -1 as a start signal and -2 ans an end signal for bulk traffic
    // using -3 as a start signal and -2 ans an end signal for interactive traffic
    // -1 <traffic_id> <traffic_src> <num_dst> [<dst_id> <recv_id>]
    for(auto& traffic : m_traffic) {
        if(traffic->_priority == CodedBulkTraffic::Interactive) {
            os << "-3 ";
        } else {
            os << "-1 ";
        }
        os << traffic->_id << " " << traffic->_src_id << " " << traffic->_dst_id.size() << " ";
        int total_dst = 0;
        for(auto& dst_id : traffic->_dst_id) {
            os << total_dst << " " << dst_id << " ";
            ++total_dst;
        }
        os << "-2" << std::endl;
    }
}

Ptr<CodedBulkTraffic>
CodedBulkTrafficManager::addCodedBulkTraffic (int src_id) {
    Ptr<CodedBulkTraffic> traffic = Create<CodedBulkTraffic> (src_id);
    traffic->_id = m_traffic_id++;
    m_traffic.push_back(traffic);
    return traffic;
}

void
CodedBulkTrafficManager::SetTrafficNotGenerated (bool traffic_not_generated) {
    m_traffic_not_generated = traffic_not_generated;
}

bool
CodedBulkTrafficManager::GetTrafficNotGenerated (void) {
    return m_traffic_not_generated;
}