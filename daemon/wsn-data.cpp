#include "wsn-data.hpp"

namespace nfd{
	namespace gateway{
	std::ostream &operator <<(std::ostream &os,const WsnData &wsndata) {
		os<<"Content:"<<wsndata.data<<" time:"<<wsndata.local_time<<std::endl;
		return os;
	}

	void WsnData::initialize(){
		std::string::size_type data_scope_beg,data_scope_mid,data_scope_end;
		data_scope_beg=data.find('/');
		data_scope_mid=data.find('/',data_scope_beg+1);
		data_scope_end=data.find('/',data_scope_mid+1);

		std::string data_position=data.substr(data_scope_beg+1,data_scope_mid-data_scope_beg-1);
		
		position_x=std::stoi(data_position.substr(0,data_position.find(',')));
		position_y=std::stoi(data_position.substr(data_position.find(',')+1));
		wsn_time=std::stoi(data.substr(data_scope_mid+1,data_scope_end-data_scope_mid-1));
		type=data.substr(data_scope_end+1,data.rfind('/')-data_scope_end-1);
//		std::cout<<"WSN-DATA is "<<position_x<<" , "<<position_y<<"/"<<wsn_time<<"/"<<type<<std::endl;
	}
	
	}
}
