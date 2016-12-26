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

#include "location-route-strategy.hpp"
#include "pit-algorithm.hpp"
#include "../table/pit-entry.hpp"
#include "../table/fib-entry.hpp"

namespace nfd {
namespace fw {

const Name LocationRouteStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/location-route/%FD%00");
NFD_REGISTER_STRATEGY(LocationRouteStrategy);

LocationRouteStrategy::LocationRouteStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}
LocationRouteStrategy::~LocationRouteStrategy()
{
}

void
LocationRouteStrategy::afterReceiveInterest(const Face& inFace,
                                        const Interest& interest,
                                        shared_ptr<fib::Entry> fibEntry,
                                        shared_ptr<pit::Entry> pitEntry)
{

  std::cout<<"receive a nfd location Interest"<<std::endl;
  if (hasPendingOutRecords(*pitEntry)) {
    // not a new Interest, don't forward
    return;
  }

  /* modified by ywb*/
  std::string interest_name = interest.getName().toUri();
  std::cout<<"interest is :"<<interest_name<<std::endl;
  std::string::size_type x_start=interest_name.find('/',1);
  std::string::size_type x_end = interest_name.find('/',x_start+1);
  std::string::size_type y_start=x_end;
  std::string::size_type y_end=interest_name.find('/',y_start+1);
  std::string leftUp = interest_name.substr(x_start+1,x_end-x_start-1);
  std::string rightDown = interest_name.substr(y_start+1,y_end-y_start-1);

//  std::string::size_type replace_idx=leftUp.find("%2C");
//  leftUp.replace(replace_idx,3,",");
//  replace_idx=rightDown.find("%2C");
//  rightDown.replace(replace_idx,3,",");
//  std::cout<<leftUp<<" , "<<rightDown<<std::endl;

  std::string::size_type comma_idx= leftUp.find("%2C");


  int leftUp_x = std::stoi(leftUp.substr(0,comma_idx));
  int leftUp_y = std::stoi(leftUp.substr(comma_idx+3));

  comma_idx=rightDown.find("%2C");

  int rightDown_x = std::stoi(rightDown.substr(0,comma_idx));
  int rightDown_y = std::stoi(rightDown.substr(comma_idx+3));

  std::cout<<"leftUp: "<<leftUp_x<<","<< leftUp_y<<"    rightDown: "<<rightDown_x<<","<<rightDown_y<<std::endl;

  fib::NextHopList nexthops;
  auto fib_entry_itr=m_forwarder.getFib().begin();

  for(;fib_entry_itr!=m_forwarder.getFib().end();fib_entry_itr++){
    std::ostringstream os;
    os<<(*fib_entry_itr).getPrefix()<<std::endl;
    std::string fib_entry_name=os.str();
    if(fib_entry_name.find("/nfd")!=std::string::npos && fib_entry_name.find_first_of("0123456789") != std::string::npos) {
      std::cout<<"fib name is: "<<fib_entry_name<<std::endl;
      std::string::size_type pos1=fib_entry_name.find('/',1);
      std::string::size_type pos2=fib_entry_name.find('/',pos1+1);
//      std::string::size_type pos3=fib_entry_name.find('/',pos2+1);

      int position_x = std::stoi(fib_entry_name.substr(pos1+1,pos2-pos1-1));
      int position_y = std::stoi(fib_entry_name.substr(pos2+1));
      std::cout<<"position is ("<<position_x<<","<<position_y<<")"<<std::endl;


      if(position_x >=leftUp_x && position_x <= rightDown_x  && position_y >=rightDown_y && position_y <= leftUp_y)
      {
//        fibEntry.reset(const_cast<fib::Entry*>(&*fib_entry_itr));
          nexthops=(*fib_entry_itr).getNextHops();
      }
    }
  }



//  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  fib::NextHopList::const_iterator it = std::find_if(nexthops.begin(), nexthops.end(),
    [&pitEntry] (const fib::NextHop& nexthop) { return canForwardToLegacy(*pitEntry, *nexthop.getFace()); });

//  std::cout<<(*it).getFace()<<std::endl;

  if (it == nexthops.end()) {
    this->rejectPendingInterest(pitEntry);
    return;
  }

  shared_ptr<Face> outFace = it->getFace();
  std::cout<<outFace->getRemoteUri()<<std::endl;
  this->sendInterest(pitEntry, outFace);
}

} // namespace fw
} // namespace nfd

