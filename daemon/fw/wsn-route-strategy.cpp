/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wsn-route-strategy.hpp"
#include "pit-algorithm.hpp"

namespace nfd {
namespace fw {

const Name WsnRouteStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/wsn-route/%FD%01");
NFD_REGISTER_STRATEGY(WsnRouteStrategy);

WsnRouteStrategy::WsnRouteStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

WsnRouteStrategy::~WsnRouteStrategy()
{
}

void
WsnRouteStrategy::afterReceiveInterest(const Face& inFace,
                                        const Interest& interest,
                                        shared_ptr<fib::Entry> fibEntry,
                                        shared_ptr<pit::Entry> pitEntry)
{

  std::cout<<"receive a wsn Interest"<<std::endl;
  if (hasPendingOutRecords(*pitEntry)) {
    // not a new Interest, don't forward
    return;
  }

  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  fib::NextHopList::const_iterator it = std::find_if(nexthops.begin(), nexthops.end(),
    [&pitEntry] (const fib::NextHop& nexthop) { return canForwardToLegacy(*pitEntry, *nexthop.getFace()); });

  if (it == nexthops.end()) {
    this->rejectPendingInterest(pitEntry);
    return;
  }

  shared_ptr<Face> outFace = it->getFace();
  this->sendInterest(pitEntry, outFace);
}

} // namespace fw
} // namespace nfd

