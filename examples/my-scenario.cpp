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
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/log.h"
#include "ns3/animation-interface.h"
// my header files
//#include "SimulationUtility.h"
#include "NFD/daemon/fw/my-multicast-strategy.hpp"

namespace ns3 {
  
NS_LOG_COMPONENT_DEFINE ("Scenario");
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
        posAllocator->Add(Vector(0,0,0));
        posAllocator->Add(Vector(1*(1 * 25),10,0));
        posAllocator->Add(Vector(1*(2 * 25) + 10,0,0));
        // posAllocator->Add(Vector(1*(3 * 10) + 2,-10,0));
        // posAllocator->Add(Vector(1*(4 * 10) + 10,0,0));
        mobHelper.SetPositionAllocator(posAllocator);
        mobHelper.Install(c);

        NS_LOG_INFO("Node 0" << " positions " << c.Get(0)->GetObject<ConstantPositionMobilityModel>()->GetPosition() );
    }

    int main(int argc, char* argv[])
    {   
        ns3::PacketMetadata::Enable ();
        std::string logFile = "logFile.log";
        // setting default parameters for PointToPoint links and channels
        // Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
        // Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
        // Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20p"));
        
        // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
        CommandLine cmd;
        cmd.Parse(argc, argv);
        // /home/osboxes/Sumo/manhattan/data/manhattanMobility.tcl
        //SimulationUtility simulUtils = SimulationUtility("/home/osboxes/Sumo/manhattan/data/manhattanMobility.tcl");
        //LogComponentEnable("");

        //testing values
        //uint32_t nNodes = 3;
        //double simulTime = 10;

        Ns2MobilityHelper ns2MobHelper = Ns2MobilityHelper("/home/osboxes/Sumo/manhattan/data/manhattanMobility.tcl");
        uint32_t nNodes = 30;//simulUtils.getSimulationSizeNodes();
        double simulTime = 2180;//simulUtils.getSimulationTime();
        
        std::cout << "Number of Nodes: " << nNodes << std::endl;
        //std::cout << "Entry of Node 0: " << simulUtils.getSimulationNodeEntryTime(0) << "s" << std::endl;
        //std::cout << "Exit of Node 0: " << simulUtils.getSimulationNodeExitTime(0) << "s" << std::endl;
        std::cout << "Simulation Time: " << simulTime << "s" << std::endl;

        // Creating nodes
        NodeContainer nodes;
        nodes.Create(nNodes);

        //testing mobility model
        //installTestMobility(nodes, simulTime);
        ns2MobHelper.Install();
        
        //create producer node
        Ptr<Node> prodNode = CreateObject<Node>();

        MobilityHelper mobHelper;
        mobHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        Ptr<ListPositionAllocator> posAllocator = CreateObject<ListPositionAllocator>();
        posAllocator->Add(Vector(200,100,0));
        mobHelper.SetPositionAllocator(posAllocator);
        mobHelper.Install(prodNode);

        nodes.Add(prodNode);
        std::cout << "Number of Nodes: " << nodes.GetN() << std::endl;
        //Wifi Installation
        NetDeviceContainer netDevices;

        // Create a channel helper and phy helper, and then create the channel
        YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
        YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
        phy.SetChannel(channel.Create());
        
        // Create a WifiMacHelper and configure adhoc architecture
        WifiMacHelper mac;
        mac.SetType("ns3::AdhocWifiMac");

        // Create a WifiHelper, which will use the above helpers to create
        // and install Wifi devices.  Configure a Wifi standard to use, which
        // will align various parameters in the Phy and Mac to standard defaults.
        WifiHelper wifi;
        
        wifi.SetStandard(WIFI_PHY_STANDARD_80211a); // OFDM 5GHz
        // Perform the installation of wifi on nodes
        netDevices = wifi.Install(phy, mac, nodes);

        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.SetDefaultRoutes(true);
        ndnHelper.Install(nodes);
        
        //ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/multicast");
        ndn::StrategyChoiceHelper::Install<nfd::fw::MyMulticastStrategy>(nodes, "/");
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndnGlobalRoutingHelper.InstallAll();
        
        // Installing applications
        ndn::AppHelper consumerApp("CustomApp");
        NodeContainer consumers;
        std::random_shuffle(nodes.begin(), nodes.end());
        int nConsumers = 0, maxConsumers = 5;
        std::cout << "Consumers: " << std::endl;
        for(auto it: nodes){
            std::cout << it->GetId() << " ";
            consumers.Add(it);
            if(++nConsumers == maxConsumers) 
                break;
        }
        std::cout << '\n';

        consumerApp.Install(consumers);
        
        // Consumer
        // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
        // //Consumer will request /named/0, /named/1, ...
        // consumerHelper.SetPrefix("/named");
        // consumerHelper.SetAttribute("Frequency", StringValue("1")); // 1 interests a second
        // auto app = consumerHelper.Install(nodes.Get(0));
        // app.Start(Seconds(0));
        // app.Stop(Seconds(simulTime));
        
        // ndn::AppHelper consumerApp("CustomApp");
        // auto app1 = consumerApp.Install(nodes.Get(0));
        // app1.Start(Seconds(0));
        // app1.Stop(Seconds(simulTime));
        
        // Producer
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        // Producer will reply to all requests starting with /60/0/named
        producerHelper.SetPrefix("/200/100/named");    // /x_coord/y_coord/application
        producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
        auto appProd = producerHelper.Install(nodes.Get(nodes.GetN() - 1));
        appProd.Start(Seconds(0));
        appProd.Stop(Seconds(simulTime));

        ndn::GlobalRoutingHelper::CalculateRoutes();

        // open log file for output
        std::ofstream os;
        os.open (logFile.c_str ());
        //Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeBoundCallback(&CourseChange, &os));

        Simulator::Stop(Seconds(simulTime));

        std::string animFile = "my-scenario-animFile.xml";
        AnimationInterface anim(animFile);
        ndn::AppDelayTracer::InstallAll("/home/osboxes/ndnSIM/my-simulations/app-delays-trace-2.txt");
        //L2RateTracer::InstallAll("drop-trace.txt", Seconds(0.5));
        
        Simulator::Run();
        Simulator::Destroy();

        os.close (); // close log file
        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}
