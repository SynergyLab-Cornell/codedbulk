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
#ifndef CodedBulk_TRAFFIC_H
#define CodedBulk_TRAFFIC_H

#include "ns3/openflow-routing-controller.h"
#include "CodedBulk-path.h"
#include "ns3/CodedBulk-multicast-sender.h"
#include "ns3/CodedBulk-receiver.h"
#include <list>
#include <map>

namespace ns3 {

class CodedBulkTraffic : public SimpleRefCount<CodedBulkTraffic> {
public:
    CodedBulkTraffic (int src_id);
    ~CodedBulkTraffic();

    void operator = (const CodedBulkTraffic& traffic);
    bool addDst (int dst_id);
    uint32_t GetNumDst (void);

    void IsCodedBulk (bool is_codedbulk = false);
    void ApplyCodedBulk (bool is_CodedBulk_traffic = true);
    void SetUnicast (bool is_unicast = false);
    void SetSteinerTree (bool is_Steiner_tree = false);

    enum CodedBulkTrafficPriority {
        BestEffort = 0,
        Bulk = 2,
        InteractiveBulk = 4,
        Interactive = 6
    };

    void SetPriority (CodedBulkTrafficPriority priority);

    void addWorkload (double time_ms, uint64_t size);

    Ptr<CodedBulkMultipathSet> getCodedBulkMultipathSet (int dst_id);
    void clearCodedBulkMultipathSets ();

    void listAllPathSets (std::ostream& os);

    uint64_t GetTotalMeasuredBytes (void);

    int     _id;           // traffic id
    int     _src_id;       // source router id
    std::list<int> _dst_id; // destination router id

    // destination -> multipaths
    std::map<int, Ptr<CodedBulkMultipathSet> > _path_sets;

    // corresponding applications
    Ptr<CodedBulkMulticastSender>      _sender;
    std::list<Ptr<CodedBulkReceiver> > _receivers;

    bool    _is_codedbulk    {false};
    bool    _is_CodedBulk_traffic   {true};
    bool    _is_unicast      {false};
    bool    _is_Steiner_tree {false};
    uint8_t _priority;

    typedef struct __CodedBulkTrafficLoad__ {
        double    _time_ms;
        uint64_t  _size;
    } CodedBulkTrafficLoad;

    std::list<CodedBulkTrafficLoad> _interactive_workload;
};

class CodedBulkTrafficManager : public SimpleRefCount<CodedBulkTrafficManager> {
public:
    CodedBulkTrafficManager ();
    ~CodedBulkTrafficManager ();

    Ptr<CodedBulkTraffic> addCodedBulkTraffic (int src_id);

    int  getNumCodedBulkTraffic () {  return m_traffic.size();  }
    void clearAllCodedBulkTraffic ();
    void listAllCodedBulkTraffic (std::ostream& os);

    double GetAverageThroughput (void) const;

    double GetTotalThroughput (void) const;
    double GetTotalBulkThroughput (void) const;
    double GetTotalInteractiveThroughput (void) const;

    void SetTrafficNotGenerated (bool traffic_not_generated);
    bool GetTrafficNotGenerated (void);

    std::list<Ptr<CodedBulkTraffic> >  m_traffic;
    bool m_traffic_not_generated;

private:
    static int m_traffic_id;
};

}

#endif // CodedBulk_TRAFFIC_H
