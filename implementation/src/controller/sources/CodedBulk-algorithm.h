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

#ifndef CodedBulk_ALGORITHM_H
#define CodedBulk_ALGORITHM_H

#include "openflow-routing-controller.h"
#include "CodedBulk-graph.h"
#include "CodedBulk-traffic.h"

class CodedBulkAlgorithm : public SimpleRefCount<CodedBulkAlgorithm> {
public:
  CodedBulkAlgorithm (void);
  virtual ~CodedBulkAlgorithm ();
 
  virtual void ResetParameters (void);

  void Initialize (Ptr<OpenFlowRoutingController> controller, Ptr<CodedBulkGraph> graph);

protected:
  Ptr<OpenFlowRoutingController>  m_controller;
  Ptr<CodedBulkGraph>                    m_graph;   // the network topology
};

#endif /* CodedBulk_ALGORITHM_H */
