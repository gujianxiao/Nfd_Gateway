#include "version.hpp"
#include "common.hpp"

#include <ndn-cxx/face.hpp>


namespace ndn{
	class NwdStatus{
	public:
		
	void fetchInformation()
	{
		fetch_wsn_Information();
		// fetch_wifi_Information();
	}

	void fetch_wsn_Information()
	{
		Interest interest(Name("/localhost/wsn/range"));
    	interest.setInterestLifetime(time::milliseconds(1000));
    	interest.setMustBeFresh(true);

   		 m_face.expressInterest(interest,
                           bind(&NwdStatus::wsn_onData, this,  _1, _2),
                           bind(&NwdStatus::onTimeout, this, _1));

    	// std::cout << "Sending " << interest << std::endl;

    // processEvents will block until the requested data received or timeout occurs
    	m_face.processEvents();
	}

	void fetch_wifi_Information()
	{

	}
	private:
		Face m_face;
		void onTimeout(const Interest& interest)
		{
		  std::cout << "Timeout " << interest << std::endl;
		}

		void wsn_onData(const Interest& interest, const Data& data)
		{
			// std::cout << data << std::endl;
			std::string wsn_range_data=(char*)(data.getContent().value());
			wsn_range_data.erase(wsn_range_data.find_last_of('$'));
			std::string wsn_range=wsn_range_data.substr(0,wsn_range_data.find(' '));
			int wsn_nodes_num=std::stoi(wsn_range_data.substr(wsn_range_data.find(' ')+1));
			std::cout<<"WSN:"<<std::endl;

    		std::cout <<"range:"<<wsn_range <<"\t"<<"node:"<<wsn_nodes_num;
    		std::cout<<std::endl;
		}
	};
}

int main(int argc, char const *argv[])
{
	ndn::NwdStatus nwdstatus;
	nwdstatus.fetchInformation();
	return 0;
}