//
// Created by youngwb on 3/25/17.
//

#ifndef NFD_MASTER_ROUTETABLE_ENTRY_HPP
#define NFD_MASTER_ROUTETABLE_ENTRY_HPP

#include "../coordinate.h"
namespace nfd{
    namespace gateway{
    class RouteTableEntry
    {
    public:
        enum Status {neighbor,unreachable,unknown,flood};
    private:
//        gateway::Coordinate dest_;
        gateway::Coordinate nexthop_;
        double weight_;
        Status  flag_;
    public:

        RouteTableEntry(const Coordinate &nexthop, double weight, Status flag):
            nexthop_(nexthop),weight_(weight),flag_(flag){}

        RouteTableEntry():nexthop_(),weight_(0),flag_(unknown){}

//        gateway::Coordinate get_dest() const
//        {
//            return  dest_;
//        }


        gateway::Coordinate get_nexthop() const
        {
            return nexthop_;
        }

        double get_weight() const
        {
            return weight_;
        }

        Status get_status() const
        {
            return flag_;
        }

        void set_status(Status s)
        {
            flag_=s;
        }
    };

    std::ostream& operator<< (std::ostream& os ,  const RouteTableEntry & re);
    }
}
#endif //NFD_MASTER_ROUTETABLE_ENTRY_HPP
