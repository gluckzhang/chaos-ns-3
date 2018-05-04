#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/olsr-helper.h"
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <stdio.h>
#include <math.h> 
#include "ns3/vector.h"
#include <algorithm>
using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");
#define PI 3.14159265
//Global values
//Errorinjectiontypes if true then it will initiate that kind of errorinjection 
//Generate Roads for output if RequestMapRoad is true
//Chaospath - for injecting fault in the right node for LDFI
static bool DoingChaosExperiment = false;
static bool RequestMapRoad = false;
static bool TestRunSolution = false;
static bool EnableAnimForver2 = false;
static bool EnableAnimForver2dot1 = false;
static bool EnableAnimForver2dot2 = false;
static bool SolutionFound = false;
static bool UseLogInstead = false;
//True if we want exakt solution not approximatetive ones
static bool ExactSolution = false;
static string StartNode;
static string EndNode;
static string ChaosPaths;
static vector<string> stringChaosPaths;
static vector<set<string>> GlobalCurrentWeaknesses;
static bool buildsuccess = false;
//This increase by 1 for each time a new base is set for the ip
static int ipindex = 0;
static int animindex = 0;
//this is for passing over to the controller for testing.
static string CurrentTreeInfo ;
//The name of the unknown system you want to extrapolate
static string UnknownSystemname;
//My own version of a node 
struct MyNode{
	Ptr<Node> thisnode;
	bool nodeinitialized = false;
	vector <MyNode*> neighbor;
	string nodeid;
	vector<string> IPs;
	vector<pair<string,string>> SendReceivepairs;
};

struct Build{
  vector<pair<string,string>> thisbuild;
};
//the current test system . prevbuilds is previously alternative builds , this is used in cased current build cannot continue to fulfill future weaknesses. If that happens then the system will reverse to prevbuild and try again and worst case is the vector prevbuilds is completely empty then it means that there are no solutions.
class Tree{
public:
  MyNode* startnode;
  MyNode* endnode;
  Ipv4Address startaddress;
  Ipv4Address endaddress;
  vector<Build> prevbuildeds;
  vector<vector<Build>> prevsuccessbuilds;
  vector<string> prevbuildedpoints;
  vector<int> prevbuildpsizes;
  vector<set<string>> TreeCurrentWeaknesses;
};

static vector<MyNode*> allmynodes;
//----------------------------------------------------
//Your system for chaos injection
//Useful global methods
//split a string by delimiter
vector<string> split(const string& str, const string& delim)
{
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}

//CheckResult of experiment
bool CheckResult(vector<set<string>> chaospaths,vector<set<string>> weaknesses){
  bool success = false;
  //Uncomment if you only want the exact solution
  if(ExactSolution){
		if(chaospaths.size() !=weaknesses.size()){
		  return false;
		}
  }
  for( set<string> s1 : weaknesses){
    success = false;
    for(set<string> s2 : chaospaths){
      set<string> dumset; 
      if(s2.size() > s1.size()){
        set_difference(s2.begin(),s2.end(),s1.begin(),s1.end(),inserter(dumset,dumset.begin()));
      }else{
        set_difference(s1.begin(),s1.end(),s2.begin(),s2.end(),inserter(dumset,dumset.begin()));
      }
      if(dumset.empty()){
        success = true;
        break;
      }
    }
    if(!success){
      return false;
    }
  }
  return success;
}

//Execute a cstring command in ubuntu terminal
void exec(const char* cmd) {
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    
}

vector<set<string>> MakeVecSet(string chaospaths){
  vector<set<string>> vecset;
  vector<string> paths = split(chaospaths,"|");
  for(string elem: paths){
    vector<string> dumvec = split(elem,",");
    set<string> s(dumvec.begin(),dumvec.end());
    vecset.push_back(s);
  }
  return vecset;
}

MyNode* LookUpForNs3Node(uint32_t id,vector<MyNode*>& allmynodes){
  for(MyNode* elem : allmynodes){
    if(elem->thisnode->GetId() == id){
      return elem;
    }
  }
  return NULL;
}

MyNode* LookUpForMyNode(string id,vector<MyNode*>& allmynodes){
  for(MyNode* elem : allmynodes){
    if((elem->nodeid.compare(id)==0) ){
      return elem;
    }
  }
  return NULL;
}

//Callback method when a traced udpechoclient is send a packet
void SendEventCallback(Ptr<UdpEchoClient> client){
		ostringstream oss;
		pair<string,string> pr(StartNode,EndNode);
		oss << "[PlanedSendEvent],At,time,"<< ((int)(Simulator::Now()).GetSeconds()) <<"s,EchoClientNode," << pr.first << ",Send,packages,to,EchoServerNode," << pr.second ;
		string attribute = " --TreeInfo=" + CurrentTreeInfo + " --ChaosEvent=" + oss.str() + "\" "; 
		string command = "./waf --run \"scratch/Xpolatechaoscontroller " + attribute + " 2> scratch/Xpolatechaoscontrollerlogs.txt";
		exec(command.c_str());
		exec("diff scratch/caseNetFlixlogs3UnwantedLogs.txt scratch/Xpolatechaoscontrollerlogs.txt | grep '>' | sed 's/^> //g' > scratch/Xpolatechaoscontrollerlogsdiff.txt");
		ifstream infile("scratch/Xpolatechaoscontrollerlogsdiff.txt");
		for (string line; std::getline(infile, line); ) {
		    clog << line << endl;
		    //Check if it is correct with the weaknesses we found
		    if (line.find("CHAOSPATHS") != string::npos){
		      string foundstr = line.substr(13,line.length()-13);
		      vector<set<string>> vecset = MakeVecSet(foundstr);
		      cout << "GlobalCurrentWeaknesses " ;
		      for(set<string> s : GlobalCurrentWeaknesses){
		        string str = "";
		        for( string elem : s){
		          str += elem + ",";
		        }
		        str = str.substr(0,str.length()-1);
		        str += "|";
		        cout << str;
		      }
		      cout << endl;
		      if(!TestRunSolution){
				    if(CheckResult(vecset,GlobalCurrentWeaknesses)){
				      cout << "Correct " << endl;
				      buildsuccess = true;
				    }else{
				      cout << "Wrong ! " << endl;
				      buildsuccess = false;
				    }
				  }
		    }
		}
	}
	
//This animation config CaseNetFlixver2dot1
void ConfigureForVer2(AnimationInterface& anim,NodeContainer allnodes,Tree* tree){
	double r = 20;
	double angle = PI/4;
	anim.SetConstantPosition(allnodes.Get(0),0,0);
	anim.SetConstantPosition(allnodes.Get(1),r*cos(angle),r*sin(angle));
	anim.SetConstantPosition(allnodes.Get(4),r*cos(angle),0);
	anim.SetConstantPosition(allnodes.Get(3),r*cos(angle),-r*sin(angle));
	anim.SetConstantPosition(allnodes.Get(2),2*r*cos(angle),0);
}

void ConfigureForVer2dot1(AnimationInterface& anim,NodeContainer allnodes,Tree* tree){
	double r = 20;
	double angle = PI/4;
	anim.SetConstantPosition(tree->startnode->thisnode,0,0);
	anim.SetConstantPosition(allnodes.Get(6),r*cos(angle),r*sin(angle));
	anim.SetConstantPosition(allnodes.Get(1),r*cos(angle),0);
	anim.SetConstantPosition(allnodes.Get(4),r*cos(angle),-r*sin(angle));
	anim.SetConstantPosition(allnodes.Get(5),r*cos(angle) + r,r*sin(angle));
	anim.SetConstantPosition(allnodes.Get(3),r*cos(angle) + r,0);
	anim.SetConstantPosition(tree->endnode->thisnode,r*cos(angle) + 2*r,0);
	anim.SetConstantPosition(allnodes.Get(2),2*r*cos(angle),0);
}

void ConfigureForVer2dot2(AnimationInterface& anim,NodeContainer allnodes,Tree* tree){
	double r = 20;
	double angle = PI/4;
	anim.SetConstantPosition(allnodes.Get(0),0,0);
	anim.SetConstantPosition(allnodes.Get(1),r*cos(angle),r*sin(angle));
	anim.SetConstantPosition(allnodes.Get(5),r*cos(angle),0);
	anim.SetConstantPosition(allnodes.Get(2),r*cos(angle) + r,r*sin(angle));
	anim.SetConstantPosition(allnodes.Get(4),r*cos(angle) + r,0);
	anim.SetConstantPosition(allnodes.Get(3),r*cos(angle) + 2*r,0);

}
class System{

public:
   Tree* tree;
   vector<MyNode*> allmynodes;
   ~System(){};
   System(Tree* tr){
     tree = tr;
   };

	//Get IP from a node
	string GetNodeIP(Ptr<Ipv4> ipv4,uint32_t index){
		ostringstream oss;
		Ipv4InterfaceAddress iaddr = ipv4->GetAddress (index,0); 
		Ipv4Address ipAddr = iaddr.GetLocal (); 
		ipAddr.Print(oss);
		return oss.str();
	}
	//Check if the IP is in the vector
	bool FindIP(string IP,vector<string> vector){
		  if ( find(vector.begin(), vector.end(), IP) != vector.end() ){
		    return true;
		  }
		  return false;
	}

	//write out a message
	void Message(string msg){
		clog << msg << endl;
	}
	//Build an udp echo server
	void BuildEchoServer(Ptr<Node> servernode,int port){
		UdpEchoServerHelper echoServer (port);
		ApplicationContainer serverApps = echoServer.Install (servernode);
		serverApps.Start (Seconds (1.0));
		serverApps.Stop (Seconds (10.0));
	}
	//Build an udp echo client
	void BuildEchoClient(Ptr<Node> clientnode,Tree* tr,Ipv4Address address,int port,double starttime,vector<MyNode*>& allmynodes){
		UdpEchoClientHelper echoClient (address, port);
		echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
		echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
		echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

		ApplicationContainer clientApps = echoClient.Install (clientnode);
		clientApps.Start (Seconds (starttime));
		clientApps.Stop (Seconds (10.0));
		
		//uint32_t servernodeid = tr->endnode->thisnode->GetId();
		
		//for(auto elem : allmynodes){
		//	if(FindIP(oss.str(),elem->IPs)){
		//	  servernodeid = elem->thisnode->GetId();
		//	}
		//}
		//Add tracedcallback to the udpechoclient
		if(!DoingChaosExperiment){
			(clientApps.Get(0))->TraceConnectWithoutContext("MyTx",MakeCallback (&SendEventCallback));
		}
		pair<string,string> pr(tr->startnode->nodeid,tr->endnode->nodeid);
		MyNode* worknode = tr->startnode;
		worknode->SendReceivepairs.push_back(pr);
		
	}

 	//Next unvisited nodes in the queue children
  void AddChildren(vector<MyNode*>& children,vector<MyNode*> addition,vector<string> visited){
    for( auto elem: addition){
      if(!CheckVisited(elem->nodeid,visited)){
        children.push_back(elem);
      }
    }
  }
  //Check if visited
  bool CheckVisited(string nodeid,vector<string> visited){
    for(string elem : visited){
      if((nodeid.compare(elem)==0)){
        return true;
      }
    }
    return false;
  }
 	//Map roads from a start to an end node and store all of the roads in currentroad
  void MapRoad(string start,string end,vector<MyNode*>& allmynodes,vector<string> visited,string currentroad,vector<string>& roads){
	  currentroad +=  start + ",";
	  MyNode *currentnode = LookUpForMyNode(start,allmynodes);
	  visited.push_back(start);
	  for( auto elem : currentnode->neighbor){
	    string id = elem->nodeid;
	    if( id != end){
				if(!CheckVisited(id,visited)){
				  MapRoad(id,end,allmynodes,visited,currentroad,roads);
				}
		  }
		  else{
		    roads.push_back(currentroad + id);
		  }
	  }
  }


  //This is faultinjection in a node so that packet can no longer be send through it
  void SetDownNode(Ptr<Node> node){
    Ptr<Ipv4> ip = node->GetObject<Ipv4>();
	  for(uint32_t j=0; j < ip->GetNInterfaces(); ++j){
	    ip->SetDown(j);
	  }
  }
  
  //Check whether a set is in visisted
  bool CheckVis(set<int> s,vector<set<int>> visited){
    for(set<int> elem : visited){
      set<int> dumset;
      set_difference(s.begin(),s.end(),elem.begin(),elem.end(),inserter(dumset,dumset.begin()));
      //Take difference if dumset is 0 by any elem then it is true otherwise false after the entire loop
      if(dumset.empty()){
        return true;
      }
    }
    return false;
  }
  
  void BuildTree(MyNode* currentnode,Tree* tr,vector<set<int>> visited){
    MyNode* endn = tr->endnode;
    if(!(currentnode->nodeid.compare(endn->nodeid)==0)){
		  //for each neighbor put them in the same container and build device and interface
		  for(MyNode* myn : currentnode->neighbor){
		    set<int> s;
		    s.insert(s.end(),(int)currentnode->thisnode->GetId());
		    s.insert(s.end(),(int)myn->thisnode->GetId());
		    //If not visited then build it.
		    if(!CheckVis(s,visited)){
		    
		      //Uncomment if you want to know which pair is being build right now
	        //cout << "Now building the pair ";
					//cout << currentnode->nodeid << "," << myn->nodeid;
					//cout << endl;
					
				  //Create a NodeContainer of 2 adjecent nodes
				  NodeContainer nc;
				  nc.Add(currentnode->thisnode);
				  nc.Add(myn->thisnode);
				  //Create p2pdevices
					PointToPointHelper pointToPoint;
					pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
					pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
				  NetDeviceContainer ncdevs = pointToPoint.Install(nc);
				  //Create Interfaces
				  string ip = "10.1." +  to_string(ipindex) + ".0";
				  ipindex++;
				  Ipv4AddressHelper address;
				  address.SetBase (ip.c_str(), "255.255.255.0");
				  Ipv4InterfaceContainer con = address.Assign(ncdevs);
				  //Remember that we have already build a connection by putting these pairs in visited well its a set in fact for easier checking later.
				  visited.push_back(s);
				  //save ipv4 address for client and server at start and end node
				  if((currentnode->nodeid.compare(tr->startnode->nodeid)==0) ){
				    tr->startaddress = con.GetAddress(0);
				  }
	  		  if((myn->nodeid.compare(tr->endnode->nodeid)==0) ){
				    tr->endaddress = con.GetAddress(1);
				  }
				  
				  BuildTree(myn,tr,visited);
				}
		  }
    }
  }
  //Put all ns3 node Ptr<Node> on all of tree mynode and also put it in to the current global allmynodes
  void PutNodesOnTree(MyNode* currentnode,Tree* tr,NodeContainer& allnodes){
    allnodes.Create(1);
    Ptr<Node> node = allnodes.Get(allnodes.GetN()-1);
    currentnode->thisnode = node;
    currentnode->nodeinitialized = true; 
    this->allmynodes.push_back(currentnode);
    for(MyNode* elem : currentnode->neighbor){
      CurrentTreeInfo += currentnode->nodeid + "," + elem->nodeid + "|";
      if(!(elem->nodeinitialized)){
      	PutNodesOnTree(elem,tr,allnodes); 
      }
    }
    
    
  }
	void RunSystem ()
	{     
			  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
			  LogComponentEnable ("UdpEchoClientApplication", LOG_PREFIX_ALL);
			  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
			  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
				vector<string> roads;
				vector<string> visited;
				//Clear out everything
				CurrentTreeInfo = "";
				NodeContainer allnodes;
				//static vector<MyNode*> stuffs;
				
				//Create nodes for mynodes
				PutNodesOnTree(tree->startnode,tree,allnodes);
				//Create Internetstack
				InternetStackHelper internet;
				internet.Install (allnodes);
				vector<set<int>> doned;
				BuildTree(tree->startnode,tree,doned);

			  //Add mobility for animation 
        MobilityHelper mobility;
				mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
												               "MinX", DoubleValue (0.0),
												               "MinY", DoubleValue (0.0),
												               "DeltaX", DoubleValue (5.0),
												               "DeltaY", DoubleValue (5.0),
												               "GridWidth", UintegerValue (3),
												               "LayoutType", StringValue ("RowFirst"));
						                     
			  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
				mobility.Install (allnodes);
				
				
				//Animation,Placing Nodes
				string animname;
				if(!DoingChaosExperiment){
					animname = "ExtrapolatedSystems/Xpolator" + to_string(animindex) +".xml";
					++animindex;
				}else{
				  animname = "ExtrapolatedSystems/Controlleranim.xml";
				}
				AnimationInterface anim(animname);		
				
				
				for(uint32_t i=0; i < allnodes.GetN() ;++i){
				  anim.UpdateNodeColor (allnodes.Get(i), 0, 255, 0); 
				}
				//Choose one of these so that we can compare easier with the original.
				if(TestRunSolution){
				  if(EnableAnimForver2){
				    ConfigureForVer2(anim,allnodes,tree);
				  }
				  if(EnableAnimForver2dot1){
				    ConfigureForVer2dot1(anim,allnodes,tree);
				  }
				  if(EnableAnimForver2dot2){
				    ConfigureForVer2dot2(anim,allnodes,tree);
				  }
				}
				//build echoservers and echoclients
				BuildEchoServer(this->tree->endnode->thisnode,9);
				BuildEchoClient(this->tree->startnode->thisnode,this->tree,this->tree->endaddress, 9,2.0,this->allmynodes);
				//for(MyNode* elem : this->allmynodes){
				//  anim.UpdateNodeDescription(elem->thisnode,elem->nodeid);
				//}
				Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
				
			  anim.UpdateNodeDescription(tree->startnode->thisnode,"STARTNODE");
			  anim.UpdateNodeDescription(tree->endnode->thisnode,"ENDNODE");
			  if(!stringChaosPaths.empty()){
					for( string elem : stringChaosPaths){
					  MyNode* n = LookUpForMyNode(elem,this->allmynodes);
					  SetDownNode(n->thisnode);
					  anim.UpdateNodeColor(n->thisnode, 255, 0, 0); 
				  }
			  }
			  
				Ipv4GlobalRoutingHelper::RecomputeRoutingTables ();
				
			  if(RequestMapRoad){
					MapRoad(StartNode,EndNode,this->allmynodes,visited,"",roads);
					clog << "[Roads from Node " << StartNode << " to Node "<< EndNode << "]:";
					for(auto elem: roads){
						clog << elem << "|";
					}
					clog << "\n";
				}
				
				Simulator::Run ();
				Simulator::Destroy ();
				
			  for(uint32_t i = 0; i < allnodes.GetN(); ++i){
			    Ptr<Node> node = allnodes.Get(i);
			    node->Dispose();
			  }
			  for(MyNode* mnode: this->allmynodes){
			    mnode->nodeinitialized = false;
			    mnode->thisnode->Dispose();
			  }
			  ipindex = 0;
	}
};

//Convert a string vector a an int vector
vector<int> ConvertStringToInt(vector<string> stringvec){
  vector<int> intvec;
  for(string elem : stringvec){
    intvec.push_back(atoi(elem.c_str()));
  }
  return intvec;
}
//Search for MyNode object given an id
MyNode* SearchMyNode(string id, vector<MyNode*> allmynodes){
  for( auto elem : allmynodes){
    if(elem->nodeid == id){
      return elem;
    }
  }
  return NULL;
}
//Interpret a string set to a MyNode vector
vector<MyNode*> SetToMyNodes(set<string> s,vector<MyNode*> allmynodes){
  vector<MyNode*> retvec;
  for(auto elem : s){
    retvec.push_back(SearchMyNode(elem,allmynodes));
  }
  return retvec;
}

//get union of all vectors in vecvec
set<string> GetUnionOfVec(vector<vector<string>> vecvec){
  set<string> retset;
  for(vector<string> thing : vecvec){
    set<string> dummyset;
    set<string> s(thing.begin(),thing.end());
    if(retset.empty()){
      dummyset = s;
    }else{
      set_union(retset.begin(), retset.end(), s.begin(), s.end(), inserter(dummyset,dummyset.begin()));
    }
    retset = dummyset;
  }
  return retset;
}

//chaospaths format example A,B|C,D|E|F|
void SetUpMyNodes(string chaospaths,vector<MyNode*>& allmynodes){
  vector<string> paths = split(chaospaths,"|");
  vector<vector<string>> weaknesses;
  unsigned int nodenumbers = 0;
  //split all the string to vector string by , and calculate the numbers of total nodes.
  for(auto elem : paths){
    vector<string> dumvector = split(elem,",");
    weaknesses.push_back(dumvector);
    nodenumbers += dumvector.size();
  }
  set<string> unionofchaos = GetUnionOfVec(weaknesses);
  //allnodes.Create(nodenumbers);
  //Create all mynodes of the nodes according to the weaknesses
  unsigned int i = 0;
  for(string elem: unionofchaos){
    MyNode* mnode = new MyNode;
    mnode->nodeid = elem;
    allmynodes.push_back(mnode);
    ++i;
  }
}


//Gives back possible injection points from weakness(inpoints),injectionpoints is previous builded  weakpoints
pair<set<string>,set<string>> GetInPoints(set<string> inpoints,vector<string> injectionpoints){
  set<string> setinjectpoints(injectionpoints.begin(),injectionpoints.end());
  set<string> retset1;
  set<string> retset2;
  //find intersection of the weakness and the previous injectionpoints
  set<string> intersect;
  set_intersection(setinjectpoints.begin(),setinjectpoints.end(),inpoints.begin(),inpoints.end(),inserter(intersect,intersect.begin()));
  //weakpoints calculation
  set_difference(inpoints.begin(),inpoints.end(), intersect.begin(),intersect.end(),inserter(retset1,retset1.begin()));
  //injectionpoints calculation
  set_difference(setinjectpoints.begin(),setinjectpoints.end(), intersect.begin(),intersect.end(),inserter(retset2,retset2.begin()));
  
  return pair<set<string>,set<string>>(retset1,retset2);
}

//cpaths have the format A,B|C,D|E|F|, make A,B|C,D| to vector<set<string>>
vector<set<string>> GetWeaknesses(string cpaths){
  vector<set<string>> Weaknesses;
  vector<string> vec = split(cpaths.substr(0,cpaths.length()-4),"|");
  for(auto elem : vec){
     vector<string> dumvec = split(elem,",");
     set<string> s(dumvec.begin(),dumvec.end());
     Weaknesses.push_back(s);
  }
  return Weaknesses;
}

bool CheckIfTaken(string elem, vector<string> vec){
   for(string thing : vec){
     if(elem==thing){
       return true;
     }
   }
   return false;
}

//Make a pair of a weakpoint and a injectionpoint and save it in the vector pair for example injectpoints A,B,C  weakpoints 1,2 means firstvec in the vecpar is A1,B1,C1,A2,B2,C2 each is a pair and combine these A1,A2|A1,B2| ... despite that it called a build object but it is not yet just a vec set . |A1,B1,C1| |A2,B2,C2| each in this case is a build object. 
vector<Build> MakeConPair(set<string> weakpoints, set<string> injectpoints){
  vector<Build> vecvecpar;
  vecvecpar.resize(weakpoints.size());
  vector<string> weakpointsvec(weakpoints.begin(),weakpoints.end());
  for( string elem1 : injectpoints){
    for(unsigned int i = 0; i < weakpointsvec.size(); ++i){
      vecvecpar[i].thisbuild.push_back(pair<string,string>(elem1,weakpointsvec[i]));
    }
  }
  return vecvecpar;
}
//Generate combinations given 2 vectors of pairs . 
vector<Build> GenerateCombo(vector<Build> vecpair1,Build vecpair2){
  vector<Build> retvecvecpair;
  for(pair<string,string> elem1: vecpair2.thisbuild){
    for(Build elem2 : vecpair1){
      Build b;
    	vector<pair<string,string>> dumvec(elem2.thisbuild.begin(),elem2.thisbuild.end());
    	dumvec.push_back(elem1);
    	b.thisbuild = dumvec;
    	retvecvecpair.push_back(b);
    }
  }
  return retvecvecpair;
}

//every vector of pairs is a build. Rememeber that each Build in conpairs is of format A1,A2,A3|B1,B2,B3... . Each is a connection between a injectionpoint(A,B) and a weakpoint(1,2,3)
vector<Build> GenerateBuilds(vector<Build> conpairs){
  vector<Build> builds;
  for(Build elem : conpairs){
		if(builds.empty()){
		  for(pair<string,string> par : elem.thisbuild){
		    Build b;
		    b.thisbuild.push_back(par);
		  	builds.push_back(b);
		  }
		}else{
		  builds = GenerateCombo(builds,elem);
		}
  }
  return builds;
}

void ConnectToEndNode(Build b,vector<MyNode*> allmynodes,Tree* tr){
  //write out the current build which will be tested
  cout << "EXPERIMENTING BUILD FORMAT<INJECTIONPOINT,WEAKPOINT>: " ;
  for(pair<string,string> par : b.thisbuild){
    cout << "<"<<par.first << "," << par.second << "> ";
  }
  cout << endl;
  
  for(string str : tr->prevbuildedpoints){
    MyNode* mnode = SearchMyNode(str,allmynodes);
    if((str.compare(tr->endnode->nodeid)!=0) & (str.compare(tr->startnode->nodeid)!=0) ){
		  if(((int)mnode->neighbor.size()==1 )){
		    MyNode* end = tr->endnode;
		 	  end->neighbor.push_back(mnode);
		 	  mnode->neighbor.push_back(end);
		  }
		}
  }
}

void DisconnectToEndNode(Build b,vector<MyNode*> allmynodes,Tree* tr){
  MyNode* end = tr->endnode;
  for(MyNode* mnode : end->neighbor){
    mnode->neighbor.pop_back();
    end->neighbor.pop_back();
  }
}
//Build experiment from generated hypotheses. 
void BuildExperiment(Build b,vector<MyNode*> allmynodes,Tree* tr){
  for( pair<string,string> par: b.thisbuild){
    MyNode* node1 = SearchMyNode(par.first,allmynodes);
    MyNode* node2 = SearchMyNode(par.second,allmynodes);
    node1->neighbor.push_back(node2);
    node2->neighbor.push_back(node1);
  }
}

//Kill the builded experiment to revert the system back to previous state
void KillExperiment(Build b,vector<MyNode*> allmybuildnodes,Tree* tr){
  for( pair<string,string> par: b.thisbuild){
    MyNode* node1 = SearchMyNode(par.first,allmybuildnodes);
    MyNode* node2 = SearchMyNode(par.second,allmybuildnodes);
    node1->neighbor.pop_back();
    node2->neighbor.pop_back();
  }
}

//Produce all of the build with the correct weaknesses and put it in the tree.
void ProduceSuccessBuilds(Tree* tr,vector<MyNode*>& allmybuildnodes,vector<Build> builds){
  tr->prevsuccessbuilds.resize(tr->prevsuccessbuilds.size() + 1);
  GlobalCurrentWeaknesses = tr->TreeCurrentWeaknesses;
  //Test out and find all successful builds
  for(Build b: builds){
    buildsuccess = false;
    BuildExperiment(b,allmybuildnodes,tr);
    ConnectToEndNode(b,allmybuildnodes,tr);
    System mysyst(tr);
    mysyst.RunSystem();
    if(buildsuccess){
      tr->prevsuccessbuilds[tr->prevsuccessbuilds.size() - 1].push_back(b);
    }
    DisconnectToEndNode(b,allmybuildnodes,tr);
    KillExperiment(b,allmybuildnodes,tr);
  }
  GlobalCurrentWeaknesses.clear();
}


//Return a pair of weakness and injectionpoints plus the builds from those. par.first is weaknesspoints and par.second is injectionpoints
pair<vector<Build>,pair<set<string>,set<string>>> MakeBuildsFromWeakness(Tree* tr,vector<set<string>>& futureweaknesses){
  //par.first is weaknesspoints and par.second is injectionpoints
  pair<set<string>,set<string>> par = GetInPoints(futureweaknesses[0],tr->prevbuildedpoints);
  cout << "TRYING TO INCLUDE THE WEAKNESS : " ;
  for(string str : futureweaknesses[0]){
    cout << str << " " ;
  }
  cout << endl;
  cout << "|WEAKPOINTS : " ;
  for( string str : par.first){
    cout << str << " ";
  }
  cout << "|INJECTIONPOINTS : " ;
  for( string str : par.second){
    cout << str << " ";
  }
  cout << endl;
  //Set up build, every vector in hypothesis is a build 
  vector<Build> conpairs;
  vector<string> v(par.first.begin(),par.first.end());
  conpairs = MakeConPair(par.first,par.second);

  //Generate all builds from the weak and inject points.
  vector<Build> builds;
  builds = GenerateBuilds(conpairs);
  return pair<vector<Build>,pair<set<string>,set<string>>>(builds,par);
}

void DoExperiment(Tree* tr,vector<MyNode*> allmybuildnodes,vector<set<string>>& FutureWeaknesses){
  if(!FutureWeaknesses.empty()){
		//Make Builds and a pair of weakness and injectionpoints
		vector<Build> builds;
		pair<vector<Build>,pair<set<string>,set<string>>> retpair = MakeBuildsFromWeakness(tr,FutureWeaknesses);
		builds = retpair.first;
		pair<set<string>,set<string>> par = retpair.second;
 		//save weakpoints as new injectionpoints after successfully builded as well as saving the new weakness. Also save the current size of thebuildedpoints so we can use for removal if it is a failure.
		tr->TreeCurrentWeaknesses.push_back(FutureWeaknesses[0]);
		FutureWeaknesses.erase(FutureWeaknesses.begin());
		tr->prevbuildedpoints.insert(tr->prevbuildedpoints.end(),par.first.begin(),par.first.end());
		tr->prevbuildpsizes.push_back((int)tr->prevbuildedpoints.size());
		//Produce successfull build to the current tree(added to prevsuccessfullbuild)
		ProduceSuccessBuilds(tr,allmybuildnodes,builds);
		//Check if success or fail , the Tree latest prevsuccessbuild is empty or not .
		//This is failure. Destroy the empty vec and after reverseback to previous state take a previous successbuild and try again.
		if(tr->prevsuccessbuilds[tr->prevsuccessbuilds.size() - 1].empty()){
		  if(tr->prevsuccessbuilds.size() == 1 ){
				  tr->prevsuccessbuilds.clear();
				  cout << "prevsuccessbuilds size : " << tr->prevsuccessbuilds.size() << endl;
				  goto label;
				}else{
				  tr->prevsuccessbuilds.resize(tr->prevsuccessbuilds.size() - 1);
				}
				
			while(tr->prevsuccessbuilds[tr->prevsuccessbuilds.size() - 1].empty()){
				if(tr->prevsuccessbuilds.size() == 1 ){
				  tr->prevsuccessbuilds.clear();
				  cout << "prevsuccessbuilds size : " << tr->prevsuccessbuilds.size() << endl;
				}else{
				  tr->prevsuccessbuilds.resize(tr->prevsuccessbuilds.size() - 1);
				}
		    //Erase the previous build. Also erase currentweaknesses and put it as futureweaknesses. prevbuildedpoints must also be removed since we already demolish the build.
		    cout << "Reversing build since the previous failed" << endl;
				Build previous = tr->prevbuildeds.back();
				tr->prevbuildeds.pop_back();
				
				cout << "Previous build : ";
				for(pair<string,string> par : previous.thisbuild){
					cout << "<" << par.first << "," << par.second << "> "; 
				}
				cout << endl;
				
				KillExperiment(previous,allmybuildnodes,tr);
				set<string> weak = tr->TreeCurrentWeaknesses.back();
				tr->TreeCurrentWeaknesses.pop_back();
				FutureWeaknesses.insert(FutureWeaknesses.begin(),weak);
				//difference of the current and the previous step is how much we should remove from prevbuildedpoints
				int back;
				if(tr->prevbuildpsizes.size()==1){
					back = tr->prevbuildpsizes[tr->prevbuildpsizes.size() - 1];
					tr->prevbuildpsizes.clear();
					cout << "prevbuildpsizes size : " << tr->prevbuildpsizes.size() << endl;
				}
				else{
					back = tr->prevbuildpsizes[tr->prevbuildpsizes.size() - 1] - tr->prevbuildpsizes[tr->prevbuildpsizes.size() - 2];
					tr->prevbuildpsizes.pop_back();
				}
				tr->prevbuildedpoints.erase(tr->prevbuildedpoints.end() - back,tr->prevbuildedpoints.end());
				
			}
			//Try out another build in the previous success level if the previous level is not empty
			label:
				if(!tr->prevsuccessbuilds.empty()){
					Build tryanotherbuild = tr->prevsuccessbuilds[tr->prevsuccessbuilds.size() - 1].back();
					tr->prevsuccessbuilds[tr->prevsuccessbuilds.size() - 1].pop_back();
					BuildExperiment(tryanotherbuild,allmybuildnodes,tr);
					DoExperiment(tr,allmybuildnodes,FutureWeaknesses);
				}else{
					cout << "No solution or exact solutions found for these weaknesses" << endl;
				}
		}
		//This is success. Take one of the solution in this non empty vec and build the tree and continue to the next weakness.
		else{
	 		tr->prevbuildeds.push_back(tr->prevsuccessbuilds[tr->prevsuccessbuilds.size() - 1].back());
	 		tr->prevsuccessbuilds[tr->prevsuccessbuilds.size() - 1].pop_back();
	 		BuildExperiment(tr->prevbuildeds.back(),allmybuildnodes,tr);
			//Do experimentent again recursively as long as FutureWeaknesses is not 0.
			DoExperiment(tr,allmybuildnodes,FutureWeaknesses);
		}
  }
  else{
    cout << "Experiment done ! Solution found! " << endl;
    SolutionFound = true;
  }
}


void BuildTreeFromTreeInfo(Tree* tr,vector<MyNode*>& allmybuildnodes){
  vector<string> infovec = split(CurrentTreeInfo,"|");
  set<string> nodesinfo;
  //take union so that we get all unique nodeid . This is used for create allmybuildnodes
  for(string str : infovec){
    vector<string> v = split(str,",");
    set<string> s(v.begin(),v.end());
    set<string> dumset;
    if(nodesinfo.empty()){
      nodesinfo = s;
    }else{
      set_union(s.begin(),s.end(),nodesinfo.begin(),nodesinfo.end(),inserter(dumset,dumset.begin()));
      nodesinfo = dumset;
    }
  }
  //Create a mynode for each existed element and also put start/endnode on tree
  for(string elem : nodesinfo){
    MyNode* mnode = new MyNode;
    mnode->nodeid = elem;
    allmybuildnodes.push_back(mnode);
    if(StartNode.compare(elem)==0){
      tr->startnode = mnode;
    }
    if(EndNode.compare(elem)==0){
      tr->endnode = mnode;
    }
  }
  //Now Create neighbors to all the nodes according to infovec. format (currentnode,neigbor)
  for(string str : infovec){
    vector<string> v = split(str,",");
    MyNode* currentnode = SearchMyNode(v[0],allmybuildnodes);
    MyNode* neighbornode = SearchMyNode(v[1],allmybuildnodes);
    currentnode->neighbor.push_back(neighbornode);
  }
}
//Gather data(success and failed hypotheses about weakness of the unknown system) from the random fault injector applied to the unknown system. The cases are given as weaknesses for example 1 2 3 |2 3 4, each is a case . These will be save as 1,2,3|2,3,4| weaknesses. pair is given back as <success,failure>.
pair<vector<set<string>>,vector<set<string>>> GatherData(string cmd){
	vector<set<string>> successcases;
	vector<set<string>> failedcases;
  //Run the random inject controller for data
  if(!UseLogInstead){
		exec(cmd.c_str());
	}
  //Read logs file and convert it back for use here
  
  bool isSuccessCases = false;
  bool isFailedCases = false;
	ifstream infile("scratch/DataFromUnknowndSystem.txt");
	for (string line; std::getline(infile, line); ) {
    if (line.find("SUCCESSFULCASES") != string::npos){
      isSuccessCases = true;
      goto label;
    }
    if (line.find("FAILEDCASES") != string::npos){
      isSuccessCases = false;
      isFailedCases = true;
      goto label;
    }
    if(isSuccessCases){
      vector<string> v = split(line," ");
			set<string> s(v.begin(),v.end());
			//Smaller set first
			successcases.insert(successcases.end(),s);
    }
    if(isFailedCases){
      vector<string> v = split(line," ");
			set<string> s(v.begin(),v.end());
			//bigger set first
			failedcases.insert(failedcases.begin(),s);
    }
    label:; 
  }
  return pair<vector<set<string>>,vector<set<string>>>(successcases,failedcases);
}
//Minimize the set by take a start point and take intersection with the rest in the vector. If the intersection is non zero it wont be included in the next recursion.
void Minimizesuccesses(vector<set<string>> vs,int start,vector<set<string>>& modvecset){
	if((start>=(int)vs.size())){
	  modvecset = vs;
	}else{
	  vector<set<string>> retvs;
		vector<set<string>> dumvec1(vs.begin(), vs.begin() + start);
		vector<set<string>> dumvec2(vs.begin() + start , vs.end());
		//for every set in dumvec1 try to elimninate everyset in dumset 2.Definition: A set is not of min size if it contains every element of a minsize set. Definition: A set of size 1 and if it is unique then it is a min set.This means that (A,B,C) is an extension of (A,B) and (A,C) , but (A,B,C) not an extension of (A,D) nor (A,B,D)
		set<string> testset = dumvec1.back();
	  for(set<string> s2 : dumvec2){
	    set<string> dumset;
	    set_difference(s2.begin(),s2.end(),testset.begin(),testset.end(),inserter(dumset,dumset.begin()));
	    if(!(dumset.size()==(s2.size()-testset.size())) ){
	      retvs.push_back(s2);
	    }
	  }
		//if there was no elimination at all then it means that we are done
		if(retvs.size()==1){
		  retvs.insert(retvs.begin(),dumvec1.begin(),dumvec1.end());
		  modvecset = retvs;
		}else{
		  start = dumvec1.size() + 1;
		  retvs.insert(retvs.begin(),dumvec1.begin(),dumvec1.end());
		  Minimizesuccesses(retvs,start,modvecset);
		}
  }
}
int main (int argc, char *argv[])
{ Time::SetResolution (Time::NS);
  LogComponentEnable ("SecondScriptExample", LOG_LEVEL_ALL);
  LogComponentEnable ("SecondScriptExample", LOG_PREFIX_ALL);
	
  CommandLine cmd;
  
  cmd.AddValue("DoingChaosExperiment","Enable chaos experiment",DoingChaosExperiment);
  cmd.AddValue("ChaosPaths","add a chaospath",ChaosPaths);
  cmd.AddValue("RequestMapRoad","Map a road from startnode to endnode",RequestMapRoad);
  cmd.AddValue("StartNode","StartNode",StartNode);
  cmd.AddValue("EndNode","EndNode",EndNode);
  cmd.AddValue("CurrentTreeInfo","The current testing tree",CurrentTreeInfo);
  cmd.AddValue("ExactSolution","Request exact solution ",ExactSolution);
  cmd.AddValue("UnknownSystemname","The file name of the random injector controller for the unknown system",UnknownSystemname);
  cmd.AddValue("EnableAnimForver2","Enable the correct animation for caseNetFlixver2",EnableAnimForver2);
  cmd.AddValue("EnableAnimForver2dot1","Enable the correct animation for caseNetFlixver2dot1",EnableAnimForver2dot1);
  cmd.AddValue("EnableAnimForver2dot2","Enable the correct animation for caseNetFlixver2dot2",EnableAnimForver2dot2);
  cmd.AddValue("UseLogInstead","Use log instead of the generated date from random injection ",UseLogInstead);
  
  cmd.Parse (argc, argv);

  vector<string> Mypaths = split(ChaosPaths,",");
  stringChaosPaths = Mypaths;
  Tree* mytree = new Tree();
  vector<MyNode*> allmybuildnodes;	

	//Extrapolate or if it is the xpolatechaoscontroller then it will ask the xpolator about the roads of the hypothesized system.
  if(!DoingChaosExperiment ){
    //Gather data from an unknown system
		clog << "COMMENCING DATA GATHERING OF THE UNKNOWN SYSTEM! PLEASE WAIT" << endl;
		string command = "./waf --run scratch/" + UnknownSystemname + " 2> scratch/DataFromUnknowndSystem.txt";
		//Data.first is successfulcases also weaknesses and data.second is the failedcases.
		pair<vector<set<string>>,vector<set<string>>> Data = GatherData(command);
		//Minimize the successfulcases.
		vector<set<string>> modvecset;
		cout << "BEGIN MINIMIZING " << endl;
		Minimizesuccesses(Data.first,1,modvecset);
		if(!modvecset.empty()){
			string cpaths;
			for(set<string> elem : modvecset){
			  string s;
			  for(string str : elem){
			    s += str + ",";
			  }
			  s = s.substr(0,s.length()-1);
			  s += "|";
			  cpaths = s + cpaths;
			}
			cout << "STARTING CHAOS INTERPOLATION WITH WEAKNESSES : " << cpaths << endl;
			SetUpMyNodes(cpaths,allmybuildnodes);
			//Set up as a start and an end node. We dont add these as prevbuilded points, but they are includes as weakness as it is required later for checking
			string start = cpaths.substr(cpaths.length()-4,1);
			string end = cpaths.substr(cpaths.length()-2,1);
			StartNode = start;
			EndNode = end;
			set<string> dums1;
			dums1.insert(dums1.end(),start);
			set<string> dums2;
			dums2.insert(dums2.end(),end);
			mytree->TreeCurrentWeaknesses.push_back(dums1);
			mytree->TreeCurrentWeaknesses.push_back(dums2);
			
			MyNode* startn = SearchMyNode(start,allmybuildnodes);
			MyNode* endn = SearchMyNode(end,allmybuildnodes);
			mytree->prevbuildedpoints.push_back(startn->nodeid);
			mytree->prevbuildpsizes.push_back((int)mytree->prevbuildedpoints.size());
				
			mytree->startnode = startn;
			mytree->endnode = endn;

			
			vector<set<string>> FutureWeaknesses = GetWeaknesses(cpaths);
				
			DoExperiment(mytree,allmybuildnodes,FutureWeaknesses);
			//Test out the approximative or correct solution
			if(SolutionFound){
				cout << "Here goes the approximative run "<< endl;
				TestRunSolution = true;
				ConnectToEndNode(mytree->prevbuildeds.back(),allmybuildnodes,mytree);
				System mysyst(mytree);
				mysyst.RunSystem();
			}
		}else{
		  cout << "No weaknesses found " << endl;
		}
	}else{
	  BuildTreeFromTreeInfo(mytree,allmybuildnodes);
	  System mysyst(mytree);
		mysyst.RunSystem();
	}
  return 0;
}







