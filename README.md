# chaos-ns-3
A simulator of chaos engineering using NS-3

Chaos engineering is a concept or approach used in many large companies all over the world that uses distributed systems. Well known examples are Netflix, Google and Facebook. When using distributed systems a lot of different types of failures and inconveniences can occur. So in order to maintain a reliable distributed system and minimize casualty we can use chaos engineering to solve these issues.

In Netflix there is a service called “Chaos Monkey” (now a part of a larger “Simian Army”), that during working hours uses fault injection to automatically find new failures and especially deep and critical ones. Doing so engineers can fix these types of problems while already ready instead of having a failure during an inconvenient time with costly losses. This method is reliable, effective and takes into account failures “outside the box” that one maybe didn’t think of. 

Our purpose here in this Github project is a part of our Bachelor’s Thesis. We will push towards and hopefully succeed in making a NS3 (Network Simulator) code that will use chaos engineering as an approach to achieve the same types of results as the Chaos Monkey service in Netflix.

IMPORTANT!!
Before you run any program in the master branch please replace the all the header files in ns3 with those in the ns3headerfiles branch. modified files: global-route-manager.cc object.cc object.h point-to-point-net-device.cc point-to-point-net-device.h udp-echo-client.cc udp-echo-client.h



You need all the files listed in the row inorder to run.

caseNetFlixchaosver1.cc    	caseNetFlixcontroller1.cc    	caseNetFlixver2Unwantedlogs.txt

caseNetFlixchaosver2.cc     caseNetFlixcontrollerver2.cc     	caseNetFlixver2Unwantedlogs.txt

caseNetFlixchaosver2dot1.cc 	caseNetFlixcontrollerver2dot1.cc     	caseNetFlixver2Unwantedlogs.txt

caseNetFlixchaosver2dot2.cc    	caseNetFlixcontrollerver2dot2.cc     	caseNetFlixver2Unwantedlogs.txt

caseNetFlixchaosver3.cc    	caseNetFlixcontrollerver3.cc     	caseNetFlixver2Unwantedlogs.txt

