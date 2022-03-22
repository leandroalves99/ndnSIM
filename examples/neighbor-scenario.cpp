/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random> 
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/wave-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/log.h"
#include "ns3/animation-interface.h"
#include "ns3/geographic-positions.h"
// my header files
#include "NFD/daemon/fw/neighborhood-strategy.hpp"
#include "NFD/daemon/fw/best-neighbor-strategy.hpp"

#include <regex>
#include <string>

#define YELLOW_CODE "\033[33m"
#define RED_CODE "\033[31m"
#define BLUE_CODE "\033[34m"
#define BOLD_CODE "\033[1m"
#define CYAN_CODE "\033[36m"
#define END_CODE "\033[0m"
#define FILLER 219

namespace ns3 {
  
NS_LOG_COMPONENT_DEFINE ("NeighborScenario");
/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */
    class SimulationUtility
    {
        private:
            /* data */
            std::string cTraceFileName; /**< File name of the ns-2 mobility trace */
            std::vector <std::pair<double, double>> nodesSimulationTimes; /**< a map to keep track of nodes entry & exit times. */

        public:
            SimulationUtility(std::string traceFileName){
                cTraceFileName = traceFileName;
                std::ifstream inputStream; 
                inputStream.open(cTraceFileName);

                std::vector< std::pair <double,double> > nodes;

                if(inputStream.is_open()){
                    std::string line;
                    std::regex r("\\$ns_ at (\\d+.\\d) \"\\$node_\\((\\d+)\\)");
                    std::smatch match;
                    while (std::getline(inputStream,line)) { 
                        if (std::regex_search(line, match, r))
                        {
                            int nodeId = std::stoi(match[2]);
                            double timeSeconds = std::stod(match[1]);

                            if (nodeId < static_cast<int> (nodes.size()) ) {
                                //std::cout << "FOUND IT" << std::endl;
                                nodes[nodeId].second = timeSeconds;
                            }else {
                                //std::cout << "NEW" << nodeId << std::endl;
                                nodes.push_back( std::make_pair(timeSeconds,timeSeconds));
                            }
                        }
                    }
                    inputStream.close();
                }
                nodesSimulationTimes = nodes;
            }
            void
            printInfo(){
                for (auto it = nodesSimulationTimes.begin(); it != nodesSimulationTimes.end(); ++it) {
                    int index = std::distance(nodesSimulationTimes.begin(), it);
                    std::cout << "Node " << index << " ==> " << it->first << " -> " << it->second  << std::endl;
                }
            }

            int
            getSimulationSizeNodes(){
                return nodesSimulationTimes.size();
            }
            double
            getSimulationNodeEntryTime(int nodeId){
                return nodesSimulationTimes.at(nodeId).first;
            } 

            double
            getSimulationNodeExitTime(int nodeId){
                return nodesSimulationTimes.at(nodeId).second;
            }

            static bool compare(const std::pair<double, double> p1, const std::pair<double, double> p2) {
                return p1.second < p2.second;
            }
            double
            getSimulationTime(){
                return std::max_element(nodesSimulationTimes.begin(), nodesSimulationTimes.end(), compare)->second;
            }
    };

    class pBar {
        public:
            std::string firstPartOfpBar, lastPartOfpBar, pBarFiller, pBarUpdater;

            pBar(double tProgress){
                firstPartOfpBar = "[";
                lastPartOfpBar = "]";
                pBarFiller = (char) 219;
                pBarUpdater = "/-\\|";
                neededProgress = tProgress;
            }
            void update(double newProgress) {
                currentProgress = newProgress;
                amountOfFiller = (int)((currentProgress / neededProgress)*(double)pBarLength);
            }
            void print() {
                currUpdateVal %= pBarUpdater.length();
                cout << "\r" //Bring cursor to start of line
                    << firstPartOfpBar; //Print out first part of pBar
                for (int a = 0; a < amountOfFiller; a++) { //Print out current progress
                    cout << CYAN_CODE << pBarFiller ;
                }
                cout << pBarUpdater[currUpdateVal];
                for (int b = 0; b < pBarLength - amountOfFiller; b++) { //Print out spaces
                    cout << " " << END_CODE;
                }
                cout << lastPartOfpBar //Print out last part of progress bar
                    << " (" << (int)(100*(currentProgress/neededProgress)) << "%)"//This just prints out the percent
                    << flush;
                currUpdateVal += 1;
            }
        private:
            int amountOfFiller,
                pBarLength = 50, //I would recommend NOT changing this
                currUpdateVal = 0; //Do not change
            double currentProgress = 0, //Do not change
                neededProgress = 100; //I would recommend NOT changing this
        };

    // Prints actual position and velocity when a course change event occurs
    static void
    CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
    {
      Vector pos = mobility->GetPosition (); // Get position
      Vector vel = mobility->GetVelocity (); // Get velocity

      // Prints position and velocities
      *os << Simulator::Now () << " Car " << mobility->GetObject<Node>()->GetId() << " POS: x=" << pos.x << ", y=" << pos.y
          << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
          << ", z=" << vel.z << std::endl;
    }

    //testing mobility
    void
    installTestMobility(NodeContainer &c, int simulTime) {
        MobilityHelper mobHelper;
        mobHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");

        Ptr<ListPositionAllocator> posAllocator = CreateObject<ListPositionAllocator>();
        //for (int i = 0; i < 5; i++) posAllocator->Add(Vector(i * 10,0,0));
        posAllocator->Add(Vector(200,0,0));
        posAllocator->Add(Vector(200,40,0));
        //posAllocator->Add(Vector(1*(1 * 25),-10,0));
        posAllocator->Add(Vector(200,80,0));
        posAllocator->Add(Vector(200,120,0));
        // posAllocator->Add(Vector(1*(4 * 10) + 10,0,0));
        mobHelper.SetPositionAllocator(posAllocator);
        mobHelper.Install(c);

        NS_LOG_INFO("Node 0" << " positions " << c.Get(0)->GetObject<ConstantPositionMobilityModel>()->GetPosition() );
    }
    //Time Simulation Checking
    void
    TimeChecking(pBar bar, double simulTime){
        //std::cout << "Time running: " << Simulator::Now().GetSeconds() << "s" << std::endl;
        bar.update(Simulator::Now().GetSeconds());
        bar.print();

        Simulator::Schedule(Seconds(1), &TimeChecking, bar, simulTime);
    }
    int main(int argc, char* argv[])
    {   
        ns3::PacketMetadata::Enable ();
        std::string logFile = "logFile.log";

        // global attributes
        uint32_t nNodes = 120;//simulUtils.getSimulationSizeNodes();
        double simulTime = 300;//simulUtils.getSimulationTime();
        std::string scenario = "";
        float pConsumers = 10;    //in percentage (%) (10|25|50%)
        uint32_t nProducers = 2;
        uint32_t nLines = 3;

        // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
        CommandLine cmd;
        // command line attibutes
        cmd.AddValue ("n", "Number of vehicles", nNodes);
        cmd.AddValue ("s", "Simulation scenario", scenario);
        cmd.AddValue ("l", "Scnario lines", nLines);
        cmd.AddValue ("t", "Simulation time (seconds)", simulTime);
        cmd.AddValue ("c", "Percentage of Consumers", pConsumers);
        cmd.AddValue ("p", "Number of Producers", nProducers);
        cmd.Parse (argc, argv);

        std::cout << "Scenario: " << scenario << std::endl;
        std::cout << "Number of Lines: " << nLines << std::endl;
        std::cout << "Simulation Time: " << simulTime << "s" << std::endl;
        std::cout << "Number of Nodes: " << nNodes << std::endl;
        std::cout << "Number of Producers: " << nProducers << std::endl;
        std::cout << "Number of Consumers: " << nNodes * (pConsumers/100) << " ~(" << pConsumers << "%)" << std::endl;

        // /home/leandro/Sumo/manhattan/data/manhattanMobility.tcl
        SimulationUtility simulUtils = SimulationUtility("/home/osboxes/Sumo/manhattan-"+ std::to_string(nLines) +"l/manhattanMobility.tcl");
        
        //ManhattanModel -> "/home/leandro/Sumo/manhattan/data/manhattanMobility.tcl" v=30 t=2180s
        //ManhattanModel -> "/home/leandro/Sumo/manhattan-3l/data/manhattanMobility.tcl" v=60 t=4146s
        //Berlin ->"/home/leandro/Sumo/Berlin/ns2mobility.tcl" v=458 t=878s    -->testing params: c=46|115|229 (10|25|50%)
        //Guimarães ->"/home/leandro/Sumo/guimarães-300v/ns2mobility.tcl" v=326 t=814s    -->testing params: c=33|82|163 (10|25|50%)
        Ns2MobilityHelper ns2MobHelper = Ns2MobilityHelper("/home/osboxes/Sumo/manhattan-"+ std::to_string(nLines) +"l/manhattanMobility.tcl");


        // Creating nodes
        NodeContainer nodes;
        nodes.Create(nNodes);

        ns2MobHelper.Install();
        
        //Add producers nodes
        NodeContainer producerNodes;
        producerNodes.Create(nProducers);

        //Ptr<Node> prodNode = CreateObject<Node>();

        MobilityHelper mobHelper;
        mobHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        Ptr<ListPositionAllocator> posAllocator = CreateObject<ListPositionAllocator>();
        if (nLines == 6){
            posAllocator->Add(Vector(50,250,0));  //Berlin p=(1230,800) | Guimarães p=(1310,610)
            posAllocator->Add(Vector(450,250,0));
        }
        if (nLines == 3) {
            posAllocator->Add(Vector(200,100,0));
        }
        mobHelper.SetPositionAllocator(posAllocator);
        mobHelper.Install(producerNodes);

        if (nProducers == 1){
             nodes.Add(producerNodes.Get(0));
        }
        if (nProducers == 2){
            nodes.Add(producerNodes.Get(0));
            nodes.Add(producerNodes.Get(1));
        }
        

        std::cout << "Producer added!" << std::endl;

        std::cout << "Setting Wifi..." << std::endl;
        /*=====> Wifi Installation <=====*/
        NetDeviceContainer netDevices;
        
        // Create a channel helper and phy helper, and then create the channel
        YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
        YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
        
        phy.SetChannel(channel.Create());
        // 21dBm ~ 70m 
        // 24dBm ~ 100m
        // 30dBm ~ 150m
        phy.Set ("TxPowerStart", DoubleValue (24)); //Minimum available transmission level (dbm)
        phy.Set ("TxPowerEnd", DoubleValue (24)); //Maximum available transmission level (dbm)
        phy.Set ("TxPowerLevels", UintegerValue (8)); //Number of transmission power levels available between TxPowerStart and TxPowerEnd included
        
        // Create a WifiMacHelper and configure adhoc architecture
        
        NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
        wifi80211pMac.SetType("ns3::OcbWifiMac"); //  in IEEE80211p MAC does not require any association between devices (similar to an adhoc WiFi MAC)...
       
        // Create a WifiHelper, which will use the above helpers to create
        // and install Wifi devices.  Configure a Wifi standard to use, which
        // will align various parameters in the Phy and Mac to standard defaults.
        Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
        //wifi.SetStandard(WIFI_PHY_STANDARD_80211a); // OFDM 5GHz
        // Perform the installation of wifi on nodes
        netDevices = wifi80211p.Install(phy, wifi80211pMac, nodes);
        /*=====> Wifi Installation <=====*/
        std::cout << "Done setting Wifi!" << std::endl;

        std::cout << "Setting NDN..." << std::endl;
        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.SetDefaultRoutes(true);
        ndnHelper.Install(nodes);
        std::cout << "Done setting NDN!" << std::endl;

        std::cout << "Setting Strategies..." << std::endl;
        if(scenario.compare("multicast") == 0)
            ndn::StrategyChoiceHelper::Install(nodes, "/","/localhost/nfd/strategy/multicast");

        if(scenario.compare("neighbor") == 0){
            // Installing Strategies
            ndn::StrategyChoiceHelper::Install<nfd::fw::NeighborhoodStrategy>(nodes, "/neighbor");
            ndn::StrategyChoiceHelper::Install<nfd::fw::BestNeighborStrategy>(nodes, "/");

            // Installing NeighborDiscovery application in all nodes
            ndn::AppHelper appHelper("NeighborDiscovery");
            
            for (uint32_t i = 0; i < nodes.size(); i++)
            {
                Ptr<Node> n = nodes.Get(i);
                auto app = appHelper.Install(n);
                if(i >= nodes.size() - nProducers){
                    app.Start(Seconds(0));
                    app.Stop(Seconds(simulTime));
                }else{
                    app.Start(Seconds(0 /*simulUtils.getSimulationNodeEntryTime(i)*/));
                    app.Stop(Seconds(simulTime /*simulUtils.getSimulationNodeExitTime(i)*/));
                }

            }
        }
        std::cout << "Done setting Strategies!" << std::endl;

        std::cout << "Setting Consumers..." << std::endl;
        //Installing Comsumer applications
        ndn::AppHelper consumersHelper("ns3::ndn::CustomApp2");
        consumersHelper.SetAttribute("NumberProducers", IntegerValue(nProducers));
        NodeContainer consumers;
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(nodes.begin(), nodes.end() - nProducers, std::default_random_engine(seed));
        int nConsumers = 0, maxConsumers = nNodes * (pConsumers/100);   //c=33|82|163 (10|25|50%)
        std::cout << "Selected random consumers: " << std::endl;
        for(auto it: nodes){
            std::cout << it->GetId() << " " << it->GetObject<MobilityModel>()->GetPosition() << std::endl;
            consumers.Add(it);
            if(++nConsumers == maxConsumers) 
                break;
        }
        std::cout << '\n';

        //auto consumersApp = consumersHelper.Install(consumers);
        for (uint32_t i = 0; i < consumers.size(); i++)
        {
            Ptr<Node> n = consumers.Get(i);
            auto app = consumersHelper.Install(n);

            app.Start(Seconds(simulUtils.getSimulationNodeEntryTime(n->GetId())));
            app.Stop(Seconds(simulUtils.getSimulationNodeExitTime(n->GetId())));

        }
        std::cout << "Done setting Consumers!" << std::endl;

        std::cout << "Setting Producer..." << std::endl;

        if (nProducers == 1){
            //Installing Producer 1
            ndn::AppHelper producerHelper("ns3::ndn::Producer");
            producerHelper.SetPrefix("/parkinglot1");    // /application
            producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
            auto appProd = producerHelper.Install(nodes.Get(nodes.GetN() - 1));   //200,100
            appProd.Start(Seconds(0));
            appProd.Stop(Seconds(simulTime));
        }
        if (nProducers == 2){
            //Installing Producer 1
            ndn::AppHelper producer1Helper("ns3::ndn::Producer");
            producer1Helper.SetPrefix("/parkinglot1");    // /application
            producer1Helper.SetAttribute("PayloadSize", StringValue("1024"));
            auto appProd = producer1Helper.Install(nodes.Get(nodes.GetN() - 2));    //50,250
            appProd.Start(Seconds(0));
            appProd.Stop(Seconds(simulTime));

            //Installing Producer 2
            ndn::AppHelper producer2Helper("ns3::ndn::Producer");
            producer2Helper.SetPrefix("/parkinglot2");    // /application
            producer2Helper.SetAttribute("PayloadSize", StringValue("1024"));
            auto appProd2 = producer2Helper.Install(nodes.Get(nodes.GetN() - 1));   //450,250
            appProd2.Start(Seconds(0));
            appProd2.Stop(Seconds(simulTime));
        }

        std::cout << "Done setting Producer!" << std::endl;
        // open log file for output
        //std::ofstream os;
        //os.open (logFile.c_str ());
        //Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeBoundCallback(&CourseChange, &os));

        // Tracing...
        //std::string animFile = "my-scenario-animFile.xml";
        //AnimationInterface anim(animFile);
        std::string dirName = "/" + std::to_string(nNodes) +"V"+ std::to_string(static_cast<int>(pConsumers)) + "C";
        std::cout << "File Name: " << "/home/osboxes/ndnSIM/simulations/" + scenario + dirName + "/rate-trace_" + scenario + ".txt" << std::endl;
        ndn::L3RateTracer::Install(nodes, "/home/osboxes/ndnSIM/simulations/" + scenario + dirName + "/rate-trace_" + scenario + ".txt", Seconds(1));
        ndn::AppDelayTracer::Install(consumers, "/home/osboxes/ndnSIM/simulations/app-delays-trace.txt");
        pBar bar(simulTime);
        std::cout << YELLOW_CODE << BOLD_CODE << "Simulation is running: " END_CODE << std::endl;
        Simulator::Schedule(Seconds(0.0), &TimeChecking, bar, simulTime);
        Simulator::Stop(Seconds(simulTime));
        Simulator::Run();
        std::cout << std::endl;
        std::cout << RED_CODE << BOLD_CODE << "End simulation" END_CODE << std::endl;
        Simulator::Destroy();

        //os.close (); // close log file
        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}
