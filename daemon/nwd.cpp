#include "nwd.hpp"
#include "wsn-data.hpp"

namespace nfd {
	namespace gateway{
	Nwd::Nwd():m_serialManager(data_ready),m_t(m_face.getIoService()),m_tsync(m_face.getIoService()),local_timestamp(0){
//		io.run();
	}
	
	void
	Nwd::initialize()
	{
		
		m_face.setInterestFilter("/wsn/topo",
                             bind(&Nwd::Topo_onInterest, this, _1, _2),
                             ndn::RegisterPrefixSuccessCallback(),
                             bind(&Nwd::onRegisterFailed, this, _1, _2));

		
		m_face.setInterestFilter("/wsn",
									 bind(&Nwd::onInterest, this, _1, _2),
									 ndn::RegisterPrefixSuccessCallback(),
									 bind(&Nwd::onRegisterFailed, this, _1, _2));

		m_face.setInterestFilter("/wsn/location",
									 bind(&Nwd::Location_onInterest, this, _1, _2),
									 ndn::RegisterPrefixSuccessCallback(),
									 bind(&Nwd::onRegisterFailed, this, _1, _2));
		

		threadGroup.create_thread(boost::bind(&Nwd::listen_wsn_data, this, &m_serialManager));
		threadGroup.create_thread(boost::bind(&Nwd::wait_data,this));

		m_tsync.expires_from_now(std::chrono::seconds(80));
		m_tsync.async_wait(boost::bind(&Nwd::time_sync,this));
//		std::cout<<"data_set max_size is"<<data_set.max_size()<<std::endl;
		
//		threadGroup.create_thread(boost::bind(&Nwd::io_service_run,this));
        threadGroup.create_thread(boost::bind(&Nwd::manage_wsn_topo, this, &m_serialManager));
    	m_face.processEvents();
		
	}

	void
	Nwd::time_sync(){
		if(local_timestamp % (3*3600) ==0)
			m_serialManager.sync_time(local_timestamp);
		++local_timestamp;
		m_tsync.expires_from_now(std::chrono::seconds(1));//set 1s timer
		m_tsync.async_wait(boost::bind(&Nwd::time_sync,this));
	}

	void
	Nwd::Location_onInterest(const InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		std::string data_val;
		for(auto& itr:location_map){
			data_val+=itr.first;
			data_val+="->";
			data_val+=itr.second;
			data_val+="$$";
		}
	    Name dataName(interest_name);
	  
	   	shared_ptr<Data> data = make_shared<Data>();
      	data->setName(dataName);
      	data->setFreshnessPeriod(time::seconds(10));
      	data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
	 	 m_keyChain.sign(*data);

	  	std::cout << ">> D: " << *data << std::endl;
      	m_face.put(*data);
		
	}

	void
	Nwd::Topo_onInterest(const InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		std::string data_val;	
		std::cout<<"before"<<std::endl;
		for(auto & itr:topo_data){
			data_val+=itr;
			data_val+="$$";
		}
		Name dataName(interest_name);
	  
	  shared_ptr<Data> data = make_shared<Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(time::seconds(10));
      data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
	  m_keyChain.sign(*data);

	  std::cout << ">> D: " << *data << std::endl;
      m_face.put(*data);
		
	}	
	

	void
  	Nwd::onInterest(const InterestFilter& filter, const Interest& interest)
  	{
    	
		std::string interest_name = interest.getName().toUri();
		if(interest_name.find("topo") != std::string::npos)
			return ;
		if(interest_name.find("location") != std::string::npos)
			return ;
		std::cout << "<< I: " << interest << std::endl;
		
		std::string::size_type pos = 0;
        while ((pos = interest_name.find("%2C", pos)) != std::string::npos){
            interest_name.replace(pos, 3, ",");
            pos += 1;
        }
		interest_name.replace(0, 1, "");
		interest_list.insert(std::make_pair(interest_name,std::set<std::string>()));
		char tmp[1024];
        strcpy(tmp, interest_name.c_str());

		if(m_serialManager.time_belong_interest(interest_name,local_timestamp)){			
			m_serialManager.handle_interest(tmp); 
			m_t.expires_from_now(std::chrono::milliseconds(400));//set 400ms timer
			m_t.async_wait(boost::bind(&Nwd::wait_data,this));
		}else{
			search_dataset(interest_name);
		}

		
//		getGlobalIoService().run();
//		std::cout<<"after timer"<<std::endl;
  	}

  void 
  Nwd::search_dataset(std::string In_Name){
  		std::string data_ret;
		for(auto &itr:data_set){
			if(search_dataset_position(In_Name,itr)){
				if(search_dataset_time(In_Name,itr)){
					if(search_dataset_type(In_Name,itr)){
//						std::cout<<"data_set match"<<std::endl;
						data_ret+=itr.data;
						data_ret+="$$";
					}
						
				}
			}
		}
		send_data(In_Name,data_ret);
  }

  bool 
  Nwd::search_dataset_type(std::string In_Name,WsnData dataval){
	std::string data_type = In_Name.substr(In_Name.rfind('/')+1);
//	std::cout<<In_Name<<std::endl;
//	std::cout<<data_type<<std::endl;
	if(data_type == dataval.type){
//		std::cout<<"dataset type match"<<std::endl;
		return true;
	}
	return false;
  }
	
  bool
  Nwd::search_dataset_time(std::string In_Name,WsnData dataval){
	std::string::size_type in_time_beg,in_time_mid,in_time_end;
	in_time_end=In_Name.rfind('/');
	in_time_mid=In_Name.rfind('/',in_time_end-1);
	in_time_beg=In_Name.rfind('/',in_time_mid-1);
	int In_Start_Time=std::stoi(In_Name.substr(in_time_beg+1,in_time_mid-in_time_beg-1));
	int In_End_Time=std::stoi(In_Name.substr(in_time_mid+1,in_time_end-in_time_mid-1));
//	std::cout<<"start:"<<In_Start_Time<<"end:"<<In_End_Time<<std::endl;
//	std::cout<<dataval.wsn_time<<std::endl;
	if(dataval.wsn_time>=In_Start_Time && dataval.wsn_time<=In_End_Time){
//		std::cout<<"dataset time match"<<std::endl;
		return true;
	}
	return false;
  }

  bool 
  Nwd::search_dataset_position(std::string Interest,WsnData dataval){
		std::string::size_type in_scope_beg,in_scope_mid,in_scope_end;
//		std::string::size_type data_scope_beg,data_scope_end;
		in_scope_beg=Interest.find('/');
		in_scope_mid=Interest.find('/',in_scope_beg+1);
		in_scope_end=Interest.find('/',in_scope_mid+1);
//		data_scope_beg=dataval.find('/');
//		data_scope_end=dataval.find('/',data_scope_beg+1);

		std::string in_scope_leftup=Interest.substr(in_scope_beg+1,in_scope_mid-in_scope_beg-1);
		std::string in_scope_rightdown=Interest.substr(in_scope_mid+1,in_scope_end-in_scope_mid-1);
		
		int in_leftup_x=std::stoi(in_scope_leftup.substr(0,in_scope_leftup.find(',')));
		int in_leftup_y=std::stoi(in_scope_leftup.substr(in_scope_leftup.find(',')+1));
		int in_rightdown_x=std::stoi(in_scope_rightdown.substr(0,in_scope_rightdown.find(',')));
		int in_rightdown_y=std::stoi(in_scope_rightdown.substr(in_scope_rightdown.find(',')+1));

//		std::string data_position=dataval.substr(data_scope_beg+1,data_scope_end-data_scope_beg-1);

//		int data_position_x=std::stoi(data_position.substr(0,data_position.find(',')));
//		int data_position_y=std::stoi(data_position.substr(data_position.find(',')+1));

//		std::cout<<"interest is "<<in_leftup_x<<" , "<<in_leftup_y<<"/"<<in_rightdown_x<<","<<in_rightdown_y<<std::endl;
//		std::cout<<"data is "<<data_position_x<<" , "<<data_position_y<<std::endl;
		if(dataval.position_x>=in_leftup_x && dataval.position_x<=in_rightdown_x){
			if(dataval.position_y>=in_rightdown_y && dataval.position_y<=in_leftup_y){			
//				std::cout<<"data_set position match"<<std::endl;
				return true;
			}	
		}
		return false;
  }

  void
  Nwd::onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cout << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }

  void 
  Nwd::listen_wsn_data(serial_manager *sm)
  {    
  	   sm->read_data(interest_list,location_map);
	   
  }

  void 
  Nwd::wait_data(){
		
		for(auto &itr:interest_list){
		    std::string data_ret;
		    std::string In_Name;
			In_Name=itr.first;
			for(auto &data:itr.second){
//				std::cout<<data<<std::endl;
				data_ret+=data;
				data_set.insert(WsnData(data,local_timestamp)); //put in data_set
				data_ret+="$$";
			}
			search_dataset(In_Name);
//			if(!data_ret.empty()){
//				
//				send_data(In_Name,data_ret);
//			}
		}
//		for(auto &itr:data_set)
//			std::cout<<itr<<std::endl;
		
  }
  
  void
  Nwd::send_data(std::string In_Name,std::string data_val)
  {
//  	  std::cout<<data_val<<std::endl;
	
	  Name dataName(In_Name);
	  
	  shared_ptr<Data> data = make_shared<Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(time::seconds(10));
      data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
	  m_keyChain.sign(*data);

	  
		
	  std::cout << ">> D: " << *data << std::endl;
      m_face.put(*data);
	  interest_list.erase(In_Name); //delete the Interest stored in interest_list
  }

  void 
  Nwd::manage_wsn_topo(serial_manager *sm)
  {
	sm->topo_management(topo_data);
  }

  }
}
