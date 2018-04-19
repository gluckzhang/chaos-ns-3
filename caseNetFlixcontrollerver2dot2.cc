#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include <thread>
#include <chrono>
#include<bits/stdc++.h>
#include <algorithm>
using namespace std;
//test stuffs
//std::string command = "./waf --run scratch/firsterrorcontroller 2> scratch/firstlogs.txt";
//exec(command.c_str());


bool NextExperiment = true;
struct CExp{
  string name;
  string attribute;
};

struct SendEvents{
  string start;
  string end;
  string attribute;
  vector<set<string>> sendroads;
  vector<vector<string>> vectorsendroads;
  vector<set<string>> chaospaths;
};

struct MyNode{
  string info;
  vector<MyNode*> neighbor;
  set<string> stringneighbor;
  vector<vector<string>> chaosways;
};

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

void exec(const char* cmd) {
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    
}

void DoingChaos(string attribute){
   std::string command = "./waf --run \"scratch/caseNetFlixchaosver2dot2" + attribute + " 2> scratch/caseNetFlixlogs2dot2.txt";
   exec(command.c_str());
}

void ReadLog(){
  exec("diff scratch/caseNetFlixver2Unwantedlogs.txt scratch/caseNetFlixlogs2dot2.txt | grep '>' | sed 's/^> //g' > scratch/caseNetFlixver2logsdiff.txt");
  ifstream infile("scratch/caseNetFlixver2logsdiff.txt");
  bool success = true;
  for (string line; std::getline(infile, line); ) {
      clog << line << endl;
      if(line.find("Echoing packet") != std::string::npos){
        success = false;
      }
  }
  if(success){
    clog << "EXPERIMENT SUCCESS" << endl;
  }else{
    clog << "EXPERIMENT FAILED" << endl;
  }
  infile.close();
}

void Setup(vector<CExp>& Cexp ,string nameln,string attrln){
    CExp thing;
    thing.name = nameln;
    thing.attribute = attrln;
    Cexp.push_back(thing);
}


void RecordSendEvents(vector<SendEvents*>& events){
  exec("./waf --run scratch/caseNetFlixchaosver2dot2 2> scratch/caseNetFlixlogs2dot2.txt");
  ifstream infile("scratch/caseNetFlixlogs2dot2.txt");
  for (string line; std::getline(infile, line); ) {
      if (line.find("[PlanedSendEvent]") != std::string::npos) {
	      vector<string> StartEndInfos = split(line," ");
	      SendEvents* thisevent = new SendEvents;
        thisevent->start = StartEndInfos[5];
        thisevent->end = StartEndInfos[StartEndInfos.size()-1];
        events.push_back(thisevent);
      } 
  }
}

void ProduceRoads(vector<SendEvents*>& events){
    string RequestMapRoad = " --RequestMapRoad=1";
    for( auto elem: events){
      elem->attribute = RequestMapRoad + " --StartNode=" + elem->start + " --EndNode=" + elem->end + "\"";
      DoingChaos(elem->attribute);
		  ifstream infile("scratch/caseNetFlixlogs2dot2.txt");
		  string findstring = "[Roads from Node " + elem->start + " to Node " + elem->end + "]";
			for (string line; std::getline(infile, line); ) {   
		    if (line.find(findstring) != std::string::npos){
			    vector<string> Infos = split(line,":");
			    vector<string> RoadsInfos = split(Infos[1],"|");
			    for( auto road: RoadsInfos){
			      vector<string> v = split(road,",");
			      set<string> s(v.begin(), v.end());
			      elem->sendroads.push_back(s);
			      elem->vectorsendroads.push_back(v);
			    }
		    } 
			}
    }
}

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

bool CheckVisited(MyNode* node, vector<MyNode*> visited){
  for(auto elem : visited){
    if(node->info == elem->info){
      return true;
    }
  }
  return false;
}

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



set<string> ConvertNodesToStringSet(vector<MyNode*> mynodes){
  vector<string> stringmynodes;
  for( auto elem : mynodes){
    stringmynodes.push_back(elem->info);
  }
  set<string> setmynodes(stringmynodes.begin(),stringmynodes.end());
  return setmynodes;
}

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

vector<set<string>> EliminatePermutation(vector<set<string>> permutedvecset){
  vector<set<string>> newvecset;
  for(auto elem : permutedvecset){
    if(!CheckIfPermutation(elem,newvecset)){
      newvecset.push_back(elem);
    }
  }
  return newvecset;
}

vector<set<string>> MakeChaosPaths(vector<MyNode*> unioninterroadsvec,vector<set<string>> intersectedsendroads){
  vector<set<string>> solutions;
  for(auto elem : unioninterroadsvec){
    vector<set<string>> solutionspart = ChaosForOneNode(elem,intersectedsendroads);
    solutions.insert(solutions.end(),solutionspart.begin(),solutionspart.end());
  }
  return solutions;
}

string MakeAttribute(set<string> s){
  string str = " --ChaosPaths=";
  for(auto elem : s){
    str += elem + "," ;
  }
  str = str.substr(0,str.length()-1);
  str += "\"";
  return str;
}

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
		  
  	  clog << "[From Node " << elem->start << " To Node " << elem->end << "] POSSIBLE ROADS : " ;
  	  
  	  for(set<string> road : elem->vectorsendroads){
  	    string str;
  	    for(string stuff : road){
  	      str += stuff + ",";
  	    }
  	    clog << str.substr(0,str.length()-1);
  	    clog << "|";
  	  }
  	  clog << endl;
  	  clog << "[From Node " << elem->start << " To Node " << elem->end << "] CHAOSPATHS : " ;
  	  for(set<string> sol : solutions){
  	    string str;
  	    for(string stuff : sol){
  	      str += stuff + ",";
  	    }
  	    clog << str.substr(0,str.length()-1);
  	    clog << "|";
  	  }
			clog << endl;
			
      for(auto solution : solutions){
        string attribute = MakeAttribute(solution);
        clog << "COMMENCING LINAGE FAULTINJECTION " << endl;
        clog << "TESTING CHAOS HYPOTHESIS FOR CHAOS SOLUTION AT NODES : ";
        for(auto thing : solution){
          clog << thing << " " ;
        }
        clog << endl;
        DoingChaos(attribute);
        ReadLog();
      }
  	}
}

int main (){
    vector <SendEvents*> events;
    RecordSendEvents(events);
    ProduceRoads(events);
    CalculateChaosPaths(events);
    return 0;
}

