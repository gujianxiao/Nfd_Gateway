//
// Created by youngwb on 3/26/17.
//
#include "routetable-entry.hpp"
#include <iomanip>

namespace nfd {
    namespace gateway {


    std::ostream& operator << (std::ostream& os,const RouteTableEntry & re)  //为了适应路由表的打印
    {
        os<<re.get_nexthop();
        os<<std::setw(8)<<re.get_weight();
        if( re.get_reachstatus()==RouteTableEntry::neighbor)
            os<<std::setw(13)<<"neighbor";
        else if(re.get_reachstatus() == RouteTableEntry::unreachable)
            os<<std::setw(13)<<"unreachable";
        else if(re.get_reachstatus() == RouteTableEntry::unknown )
            os<<std::setw(13)<<"unknown";
        else if(re.get_reachstatus() == RouteTableEntry::reachable)
            os<<std::setw(13)<<"reachable";
        else if(re.get_reachstatus() == RouteTableEntry::minlocal)
            os<<std::setw(13)<<"minlocal";

        if(re.get_sendstatus() == RouteTableEntry::sending)
            os<<std::setw(12)<<"sending";
        else if(re.get_sendstatus() == RouteTableEntry::notsending)
            os<<std::setw(12)<<" ";
        else if(re.get_sendstatus() == RouteTableEntry::flood)
            os<<std::setw(12)<<"flooding";
        else if(re.get_sendstatus() == RouteTableEntry::received)
            os<<std::setw(12)<<"received";

        os<<std::setw(8)<<re.get_face()->getRemoteUri();
        return os;
    }
    }
}