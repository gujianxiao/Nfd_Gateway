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
        os<<std::setw(15)<<re.get_weight();
        if( re.get_status()==RouteTableEntry::neighbor)
            os<<std::setw(15)<<"neighbor";
        else if(re.get_status() == RouteTableEntry::unreachable)
            os<<std::setw(15)<<"unreachable";
        else if(re.get_status() == RouteTableEntry::unknown )
            os<<std::setw(15)<<"unknown";
        else if(re.get_status() == RouteTableEntry::flood)
            os<<std::setw(15)<<"flood";
        else if(re.get_status() == RouteTableEntry::reachable)
            os<<std::setw(15)<<"reachable";
        else if(re.get_status() == RouteTableEntry::sending)
            os<<std::setw(15)<<"sending";
        return os;
    }
    }
}