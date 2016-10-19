#ifndef NFD_GATEWAY_WSNDATA_HPP
#define NFD_GATEWAY_WSNDATA_HPP

namespace nfd{
	namespace gateway{


		class WsnData{
			public:
				std::string data;
				int position_x;
				int position_y;
				int wsn_time;
				std::string type;
				int local_time;
			public:	
				WsnData(std::string data_val,int time_val):data(data_val),local_time(time_val){
					initialize();
				}

				void initialize();
			
				bool operator < (const WsnData & rhs) const{
					if( data < rhs.data)
						return true;
					return false;
				}

//				bool operator == (const WsnData & rhs){
//					if( data == rhs.data)
//						return true;
//					return false;
//				}
		friend std::ostream & operator << (std::ostream & ,const WsnData &);
				
		};

		std::ostream & operator << (std::ostream & ,const WsnData &);

	}//gateway
}//nfd
#endif
