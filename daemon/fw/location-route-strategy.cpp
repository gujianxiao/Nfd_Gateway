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
#include "../table/fib.hpp"
#include <limits>

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

/*将兴趣包的地理位置取出，写入point的横纵坐标中*/
void
LocationRouteStrategy::getPointLocation(std::string& interest_name,std::string& point_x,std::string& point_y)
{
    std::string::size_type x_start=interest_name.find('/',1);  //当前命名为/location/x/y/,以后修改为更合适的名字
    std::string::size_type x_end = interest_name.find('/',x_start+1);
    std::string::size_type y_start=x_end;
    std::string::size_type y_end=interest_name.find('/',y_start+1);
    point_x = interest_name.substr(x_start+1,x_end-x_start-1);
    point_y = interest_name.substr(y_start+1,y_end-y_start-1);
}

void
LocationRouteStrategy::printRouteTable() const
{
    std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
    std::cout<<std::setw(15)<<"dest"<<std::setw(15)<<"nexthop"<<std::setw(15)<<"weight"<<std::setw(15)<<"status"<<std::endl;
    for(auto itr : gateway::Nwd::route_table)
    {
        std::cout<<itr.first<<itr.second<<std::endl;
    }
    std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;

}


std::vector<shared_ptr<Face>>
LocationRouteStrategy::cal_Nexthos(gateway::Coordinate& dest,shared_ptr<pit::Entry> pitEntry)
{
    std::vector<shared_ptr<Face>> ret;
    auto neighbors_list_itr=gateway::Nwd::neighbors_list.find(dest);  //首先查询邻居列表，找到直接返回
    if(neighbors_list_itr != gateway::Nwd::neighbors_list.end())//在邻居列表里
    {
        std::cout<<"邻居列表中"<<std::endl;
        ret.push_back(neighbors_list_itr->second);
        return ret; //邻居列表查询到，直接返回
    }

    //查询路由表
    double minweight  =std::numeric_limits<double>::max();
    shared_ptr<Face> minface;
    gateway::Coordinate minnexthop;
    auto route_table_itr=gateway::Nwd::route_table.begin();
    auto result=gateway::Nwd::route_table.find(dest);
    if(result!=gateway::Nwd::route_table.end())  //查找到了
    {
        if(result->second.get_status() == gateway::RouteTableEntry::unreachable) //目标不可达
        {
            std::cout<<"目标不可达"<<std::endl;
            return  ret;   //返回空的结果
        }

        //查询权值最小的路径

        auto search_range=gateway::Nwd::route_table.equal_range(dest);

        for(auto itr=search_range.first;itr!=search_range.second;++itr)
        {
            double weight  = itr->second.get_weight();    //路由表中的权值
//            std::cout<<(*itr).second.first<<std::endl;
            if(weight<minweight)
            {
                minweight=weight;
                minnexthop=itr->second.get_nexthop();
                minface=gateway::Nwd::neighbors_list.find(itr->second.get_nexthop())->second;
                route_table_itr=itr;
            }
        }

    }else
    {
        //路由表没有记录，第一次查询，需要将所有邻居节点到目标的权值计算一遍并加入到路由表中
        //需向所有邻居节点发送数据，根据返回的数据更改权值，下一次就可以直接发送到目的节点
//        if(gateway::Nwd::neighbors_list.empty())
//            getNeighborsCoordinate(pitEntry);


        for(auto itr:gateway::Nwd::neighbors_list)
        {
            double weight=gateway::distance(itr.first,dest); //计算邻居节点与目的节点的距离
            auto tmp= gateway::Nwd::route_table.insert(std::make_pair(dest,gateway::RouteTableEntry(itr.first,weight,gateway::RouteTableEntry::unknown)));  //将与目标初始值插入路由表
            if(weight<minweight)
            {
                minweight=weight;
                minface=itr.second;
                minnexthop=itr.first;
                route_table_itr=tmp;
            }
        }

    }
    if(minnexthop == gateway::Nwd::get_SelfCoordinate()) //自己即是局部最优点
    {
        std::cout<<"局部最优点  "<<minnexthop<<std::endl;
        for(auto itr:gateway::Nwd::neighbors_list){
            if(itr.first != minnexthop) {
//                std::cout<<itr.first<<std::endl;
                ret.push_back(itr.second);
            }
        }
        route_table_itr->second.set_status(gateway::RouteTableEntry::flood);
    }
    else
    {
        ret.push_back(minface);
    }
    printRouteTable();
    return  ret;

}

void
LocationRouteStrategy::getNeighborsCoordinate(shared_ptr<pit::Entry> pitEntry)
{
    auto fib_entry_itr=m_forwarder.getFib().begin();


    for(;fib_entry_itr!=m_forwarder.getFib().end();fib_entry_itr++){
        std::ostringstream os;
        os<<(*fib_entry_itr).getPrefix()<<std::endl;
        std::string fib_entry_name=os.str();
        /*查找最近路由接口并转发，目前路由前缀为/nfd,后接地理位置*/
        if(fib_entry_name.find("/nfd")!=std::string::npos && fib_entry_name.find_first_of("0123456789") != std::string::npos) {
            std::cout<<"fib name is: "<<fib_entry_name;
            std::string::size_type pos1=fib_entry_name.find('/',1);
            std::string::size_type pos2=fib_entry_name.find('/',pos1+1);
            //      std::string::size_type pos3=fib_entry_name.find('/',pos2+1);

            double position_x = std::stod(fib_entry_name.substr(pos1+1,pos2-pos1-1));
            double position_y = std::stod(fib_entry_name.substr(pos2+1));
//            std::cout<<"position is ("<<position_x<<","<<position_y<<")"<<std::endl;
            fib::NextHopList nexthops;
            nexthops=fib_entry_itr->getNextHops();
            fib::NextHopList::const_iterator it = std::find_if(nexthops.begin(), nexthops.end(), [&pitEntry] (const fib::NextHop& nexthop) { return canForwardToLegacy(*pitEntry, *nexthop.getFace()); });
            shared_ptr<Face> outFace = it->getFace();
            gateway::Nwd::neighbors_list.insert(make_pair(gateway::Coordinate(position_x,position_y),outFace));

        }
    }
}

void
LocationRouteStrategy::afterReceiveInterest(const Face& inFace,
                                        const Interest& interest,
                                        shared_ptr<fib::Entry> fibEntry,
                                        shared_ptr<pit::Entry> pitEntry)
{
    std::cout<<"*******************************××××××××××××××××××××××××××××××××"<<std::endl;
    std::cout<<"receive a nfd location Interest"<<std::endl;
    if (hasPendingOutRecords(*pitEntry)) {
    // not a new Interest, don't forward
        return;
    }

    /* modified by ywb*/
    std::string interest_name = interest.getName().toUri();
    std::cout<<"interest is :"<<interest_name<<std::endl;

    std::string point_x,point_y;
    getPointLocation(interest_name,point_x,point_y);

    double point_x_val=std::stod(point_x);
    double point_y_val=std::stod(point_y);
//    std::cout<<"point_x: "<<point_x<<"    point_y: "<<point_y<<std::endl;

    gateway::Coordinate dest(point_x_val,point_y_val);  //目标位置
    fib::NextHopList nexthops;   //下一跳的列表
    std::vector<shared_ptr<Face>> faces_to_send;

//    if(gateway::Nwd::neighbors_list.empty())  //初始化邻居列表
    getNeighborsCoordinate(pitEntry);  //暂时每次读更新邻居列表，否则当FIB条目更新时无法获取

    faces_to_send=cal_Nexthos(dest,pitEntry);  //计算并返回下一跳的fib条目
    for(auto itr:faces_to_send)
    {
//        nexthops=(*itr).getNextHops();
//        //  const fib::NextHopList& nexthops = fibEntry->getNextHops();
//        fib::NextHopList::const_iterator it = std::find_if(nexthops.begin(), nexthops.end(), [&pitEntry] (const fib::NextHop& nexthop) { return canForwardToLegacy(*pitEntry, *nexthop.getFace()); });
//        if (it == nexthops.end()) {
//            this->rejectPendingInterest(pitEntry);
//            return;
//        }

//        shared_ptr<Face> outFace = it->getFace();
        std::cout<<"send to Face : "<<itr->getRemoteUri()<<std::endl;
        this->sendInterest(pitEntry, itr);
    }

    //        fibEntry.reset(const_cast<fib::Entry*>(&*fib_entry_itr));

    //  std::cout<<(*it).getFace()<<std::endl;
    std::cout<<"*******************************××××××××××××××××××××××××××××××××"<<std::endl;

}

} // namespace fw
} // namespace nfd

