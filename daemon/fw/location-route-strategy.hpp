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

#ifndef NFD_DAEMON_FW_WSN_ROUTE_STRATEGY_HPP
#define NFD_DAEMON_FW_WSN_ROUTE_STRATEGY_HPP

#include "strategy.hpp"

#include "../coordinate.h"
#include "../nwd.hpp"
#include "../table/routetable-entry.hpp"
#include <sstream>


namespace nfd {
    namespace gateway {
        class Nwd;
        class Coordinate;
        class CoordinateHash;
        class CoordinateEqual;
        class RouteTableEntry;
    }
namespace fw {


/** \brief Best Route strategy version 1
 *
 *  This strategy forwards a new Interest to the lowest-cost nexthop
 *  that is not same as the downstream, and does not violate scope.
 *  Subsequent similar Interests or consumer retransmissions are suppressed
 *  until after InterestLifetime expiry.
 *
 *  \deprecated This strategy is superceded by Best Route strategy version 2,
 *              which allows consumer retransmissions. This version is kept for
 *              comparison purposes and is not recommended for general usage.
 */
class LocationRouteStrategy : public Strategy
{
public:

  LocationRouteStrategy(Forwarder & forwarder,const Name & name = STRATEGY_NAME);
  void getPointLocation(std::string interest_name,std::string& point_x,std::string& point_y);
  std::vector<shared_ptr<Face>> cal_Nexthos(gateway::Coordinate& ,shared_ptr<pit::Entry>);
  void getNeighborsCoordinate(shared_ptr<pit::Entry>);
  void printRouteTable() const;

    virtual
  ~LocationRouteStrategy();

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry) override;

  virtual void
  beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry) override ;

public:
  static const Name STRATEGY_NAME;
private:
//  std::unordered_map<gateway::Coordinate,Fib::const_iterator,gateway::CoordinateHash,gateway::CoordinateEqual> neighbors_list;


};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_WSN_ROUTE_STRATEGY_HPP

