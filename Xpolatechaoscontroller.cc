#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include <thread>
#include <chrono>
#include<bits/stdc++.h>
#include <algorithm>
#include "ns3/core-module.h"
using namespace std;
using namespace ns3;
//Netflix module use this to input [PlanedSendEvent] into controller
static string ChaosEvent;
//This is for building a tree later for LDFI to find weaknesses 
static string TreeInfo;
//this one is used to collect all output from the controller and give it later to the netflix module
static ostringstream oss;

//For each [PlanedSendEvent] take the info from the outputfile from the Nonchaos case and store it as a sendevent
struct SendEvents{
  string start;
  string end;
  string attribute;
  vector<set<string>> sendroads;
  vector<vector<string>> vectorsendroads;
  vector<set<string>> chaospaths;
};
//This is for calculateting chaos road . 
struct MyNode{
  string info;
  vector<MyNode*> neighbor;
  set<string> stringneighbor;
  vector<vector<string>> chaosways;
};
//Split a string by a delimiter
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
//execute an cstring input in ubuntu terminal
void exec(const char* cmd) {
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    
}
//execute a chaos experiment on Xpolator
void DoingChaos(string attribute){
   std::string command = "./waf --run \"scratch/Xpolator " + attribute + " 2> scratch/Xpolatorlogs.txt";
   exec(command.c_str());
}

//Request the netflixmodule to give back all of the possible roads to the same destination
void ProduceRoads(vector<SendEvents*>& events){
    string RequestMapRoad = "--CurrentTreeInfo=" + TreeInfo + " --DoingChaosExperiment=1 --RequestMapRoad=1";
    for( auto elem: events){
      elem->attribute = RequestMapRoad + " --StartNode=" + elem->start + " --EndNode=" + elem->end + "\"";
      DoingChaos(elem->attribute);
		  ifstream infile("scratch/Xpolatorlogs.txt");
		  string findstring = "[Roads from Node " + elem->start + " to Node " + elem->end + "]";
			for (string line; std::getline(infile, line); ) { 
		    if (line.find(findstring) != std::string::npos){
			    vector<string> Infos = split(line,":");
			    vector<string> RoadsInfos = split(Infos[1],"|");
			    for( auto road: RoadsInfos){
			      vector<string> v = split(road,",");
			      set<string> s(v.begin(), v.end());
			      elem->vectorsendroads.push_back(v);
			      elem->sendroads.push_back(s);
			    }
		    } 
			}
    }
}

//Find set intersection of all roads for the signal(packet)
set<string> FindIntersection(vector<set<string>> sendroads){
  set<string> intersect = sendroads[0];
  for( auto elem: sendroads){
    set<string> dummyset;
    set_intersection(intersect.begin(),intersect.end(),elem.begin(),elem.end(),
                  inserter(dummyset,dummyset.begin()));
    intersect.clear();
    intersect = dummyset;
  }
  return intersect;
}

//Take the different of the roads with the intersection from FindIntersection so that we have less work when calculating chaospaths
vector<set<string>> FindIntersectedSendRoad(vector<set<string>> sendroads,set<string> intersect){
	vector<set<string>> intersectedsendroads;
  for(auto road : sendroads){
    set<string> intersectroad;
  	set_difference(road.begin(),road.end(),intersect.begin(),intersect.end(),
                  inserter(intersectroad,intersectroad.begin()));
    intersectedsendroads.push_back(intersectroad);
  } 
  return intersectedsendroads;
}
//Check if a set is a permutaion of one of the set in the vector
bool CheckIfPermutation(set<string> set1,vector<set<string>> vecset){
  for( auto set2 : vecset){
		set<string> dummyset;
		if(set1.size() > set2.size()){
			set_difference(set1.begin(),set1.end(),set2.begin(),set2.end(),
							        inserter(dummyset,dummyset.begin()));
		}
		else{
			set_difference(set2.begin(),set2.end(),set1.begin(),set1.end(),
						        inserter(dummyset,dummyset.begin()));
		}
		if(dummyset.empty()){
			return true;
		}
	}
  return false;
}
//Check if a node is already visisted
bool CheckVisited(MyNode* node, vector<MyNode*> visited){
  for(auto elem : visited){
    if(node->info == elem->info){
      return true;
    }
  }
  return false;
}
//Check if the generated chaospath successfully kill all of the sendroads .
bool FailAllRoads(vector<set<string>> intersectedroads,set<string> chaoswayset){
  for(auto elem : intersectedroads){
    set<string> dummyset;
    set_intersection(elem.begin(),elem.end(),chaoswayset.begin(),chaoswayset.end(),
                  inserter(dummyset,dummyset.begin()));
    if(dummyset.empty()){
      return false;
    }
     
  }
  return true;
}

//Convert a MyNode vector to a string set
set<string> ConvertNodesToStringSet(vector<MyNode*> mynodes){
  vector<string> stringmynodes;
  for( auto elem : mynodes){
    stringmynodes.push_back(elem->info);
  }
  set<string> setmynodes(stringmynodes.begin(),stringmynodes.end());
  return setmynodes;
}
//Convert a set to a vector MyNode
vector<MyNode*> ConvertSetToNodes(set<string> s){
  vector<MyNode*> mynodes;
  for(auto elem : s){
    MyNode* node = new MyNode;
    node->info = elem;
    mynodes.push_back(node);
  }
  for(int i = 0; i <(int) mynodes.size();++i){
    vector<MyNode*> vec;
    if((i-1) == -1){
			vec.insert(vec.end(),mynodes.begin() + 1,mynodes.end());
  	}
  	else if((i+1) ==(int) mynodes.size()){
  	  vec.insert(vec.end(),mynodes.begin(),mynodes.end() - 1);
  	}
  	else{
  	  vec.insert(vec.end(),mynodes.begin(),mynodes.begin() + i);
    	vec.insert(vec.end(),mynodes.begin() + i + 1,mynodes.end());
		}
    mynodes[i]->neighbor = vec; 
  }
  for(auto elem : mynodes){
    elem->stringneighbor = ConvertNodesToStringSet(elem->neighbor);
  }
  return mynodes;
}


//Generate next generation of a chaospath if the chaospath previously did not kill all sendroads
void AddChildren(vector<vector<MyNode*>>& children,vector<MyNode*> prevpath){
  for(auto elem : prevpath[prevpath.size()-1]->neighbor){
  	if(!CheckVisited(elem,prevpath)){
  	  vector<MyNode*> dummyvector2;
  		dummyvector2.insert(dummyvector2.end(),prevpath.begin(),prevpath.end());
  		dummyvector2.push_back(elem);
  		children.push_back(dummyvector2);
  	}	
  }
}

//Find all the shortest chaospath for one node
vector<set<string>> ChaosForOneNode(MyNode* node,vector<set<string>> intersectedsendroads){
  vector<vector<MyNode*>> children;
  vector<MyNode*> dummyvector1;
  vector<set<string>> solutions;
  dummyvector1.push_back(node);
  children.push_back(dummyvector1);
  int minsize = 100;
  while(!children.empty()){
    vector<MyNode*> dummyvector2 = children.front();
    children.erase(children.begin(),children.begin()+1);
    if((int)dummyvector2.size() > minsize){
      break;
    }
    set<string> dummyset = ConvertNodesToStringSet(dummyvector2);
  	if(FailAllRoads(intersectedsendroads,dummyset)){
  	  minsize = dummyset.size();
  		solutions.push_back(dummyset);
  	}
  	AddChildren(children,dummyvector2);
  }
  return solutions;
}
//Kill all of the permutations in a vector of sets
vector<set<string>> EliminatePermutation(vector<set<string>> permutedvecset){
  vector<set<string>> newvecset;
  for(auto elem : permutedvecset){
    if(!CheckIfPermutation(elem,newvecset)){
      newvecset.push_back(elem);
    }
  }
  return newvecset;
}
//Find all the shortest chaospath for all nodes
vector<set<string>> MakeChaosPaths(vector<MyNode*> unioninterroadsvec,vector<set<string>> intersectedsendroads){
  vector<set<string>> solutions;
  for(auto elem : unioninterroadsvec){
    vector<set<string>> solutionspart = ChaosForOneNode(elem,intersectedsendroads);
    solutions.insert(solutions.end(),solutionspart.begin(),solutionspart.end());
  }
  return solutions;
}
//Make attribute so that we can use it in DoingChaos later
string MakeAttribute(set<string> s){
  string str = "--DoingChaosExperiment=1 --ChaosPaths=";
  for(auto elem : s){
    str += elem + "," ;
  }
  str = str.substr(0,str.length()-1);
  str += "\"";
  return str;
}
//Calculate chaospath for each sendevent
void CalculateChaosPaths(vector<SendEvents*>& events){
  	for(auto elem: events){
		  vector<set<string>> chaospaths;
		  set<string> intersect = FindIntersection(elem->sendroads);
		  vector<set<string>> intersectedsendroads = FindIntersectedSendRoad(elem->sendroads,intersect);
		  set<string> unionintersectedroads = intersectedsendroads[0];
		  for( auto thing : intersectedsendroads){
		    set_difference(thing.begin(),thing.end(),unionintersectedroads.begin(),unionintersectedroads.end(),inserter(unionintersectedroads,unionintersectedroads.begin()));
		  }
		  
		  vector<MyNode*> unioninterroadsvec = ConvertSetToNodes(unionintersectedroads);
		  vector<set<string>> permutedsolutions = MakeChaosPaths(unioninterroadsvec,intersectedsendroads);
		  vector<set<string>> solutions = EliminatePermutation(permutedsolutions);
		  
			for(string thing : intersect){
		    vector<string> dumvec;
		    dumvec.push_back(thing);
		    set<string> dumset(dumvec.begin(),dumvec.end());
		    solutions.push_back(dumset);
		  }
		  
  	  oss << "CHAOSPATHS : " ;
  	  for(set<string> sol : solutions){
  	    string str;
  	    for(string stuff : sol){
  	      str += stuff + ",";
  	    }
  	    oss << str.substr(0,str.length()-1);
  	    oss << "|";
  	  }
			oss << endl;
			

  	}
}
//interprete chaosevent from the module to a sendevent
void EtablishSendEvent(vector <SendEvents*>& events,string ChaosEvent){
  vector<string> StartEndInfos = split(ChaosEvent,",");
  SendEvents* thisevent = new SendEvents;
  thisevent->start = StartEndInfos[5];
  thisevent->end = StartEndInfos[StartEndInfos.size()-1];
  events.push_back(thisevent);
}

int main (int argc, char *argv[]){
		CommandLine cmd;
		
    cmd.AddValue("ChaosEvent","ChaosEvent",ChaosEvent);
    cmd.AddValue("TreeInfo","Info about how to build the Tree",TreeInfo);
    cmd.Parse (argc, argv);

    vector <SendEvents*> events;
    
    EtablishSendEvent(events,ChaosEvent);
    ProduceRoads(events);
    CalculateChaosPaths(events);
    clog << oss.str() ;
    return 0;
}

