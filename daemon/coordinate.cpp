//
// Created by youngwb on 3/23/17.
//

#include "coordinate.h"

namespace nfd {
    namespace gateway {

    double distance(const Coordinate& c1, const Coordinate& c2)
    {
        double ret=abs(c1.get_longitude()-c2.get_longitude())*abs(c1.get_longitude()-c2.get_longitude())+abs(c1.get_latitude()-c2.get_latitude())*abs(c1.get_latitude()-c2.get_latitude());
        return sqrt(ret);
    }

    std::ostream& operator << (std::ostream& output,const Coordinate& c)
    {
        output<<"("<<c.get_longitude()<<" , "<<c.get_latitude()<<")"<<std::endl;
        return  output;
    }


    }
}