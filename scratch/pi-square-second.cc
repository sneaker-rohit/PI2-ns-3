/*
 * This script simulates light TCP traffic for PI Square evaluation
 * Authors: Rohit P. Tahiliani, Hitesh Tewari
 * Trinity College Dublin
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"
#include <string>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PiSquareSecond");

std::stringstream filePlotQueueDel;

uint32_t i = 0;

void CheckQueueDel (Ptr<QueueDisc> queue)
{
  double qDel = StaticCast<PiSquareQueueDisc> (queue)->GetQueueDelay ().GetSeconds ();
  // check queue delay every 30ms
  Simulator::Schedule (Seconds (0.03), &CheckQueueDel, queue);

  std::ofstream fPlotQueueDel (filePlotQueueDel.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueDel << Simulator::Now ().GetSeconds () << " " << (qDel * 1000) << std::endl;
  fPlotQueueDel.close ();
}

int main (int argc, char *argv[])
{
  bool printPiStats = true;
  bool  isPcapEnabled = true;
  float startTime = 0.0;
  float simDuration = 100;      // in seconds
  std::string  pathOut = ".";
  bool writeForPlot = true;
  std::string pcapFileName = "pi-square-second.pcap";

  float stopTime = startTime + simDuration;

  CommandLine cmd;
  cmd.Parse (argc,argv);

  LogComponentEnable ("PiSquareQueueDisc", LOG_LEVEL_INFO);

  std::string bottleneckBandwidth = "10Mbps";
  std::string bottleneckDelay = "50ms";

  std::string accessBandwidth = "10Mbps";
  std::string accessDelay = "5ms";

  NodeContainer source;
  source.Create (50);

  NodeContainer gateway;
  gateway.Create (2);

  NodeContainer sink;
  sink.Create (1);

  Config::SetDefault ("ns3::PiSquareQueueDisc::MeanPktSize", UintegerValue (1000));
  Config::SetDefault ("ns3::PiSquareQueueDisc::Mode", StringValue ("QUEUE_MODE_BYTES"));
  Config::SetDefault ("ns3::PiSquareQueueDisc::QueueDelayReference", TimeValue (Seconds (0.02)));
  Config::SetDefault ("ns3::PiSquareQueueDisc::QueueLimit", UintegerValue (200 * 1000));
 
  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.InstallAll ();

  TrafficControlHelper tchPfifo;
  uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));

  TrafficControlHelper tchPi;
  tchPi.SetRootQueueDisc ("ns3::PiSquareQueueDisc");

  // Create and configure access link and bottleneck link
  PointToPointHelper accessLink;
  accessLink.SetQueue ("ns3::DropTailQueue");
  accessLink.SetDeviceAttribute ("DataRate", StringValue (accessBandwidth));
  accessLink.SetChannelAttribute ("Delay", StringValue (accessDelay));

  NetDeviceContainer devices[50];
  for (i = 0; i < 50; i++)
    {
      devices[i] = accessLink.Install (source.Get (i), gateway.Get (0));
      tchPfifo.Install (devices[i]);
    }

  NetDeviceContainer devices_sink;
  devices_sink = accessLink.Install (gateway.Get (1), sink.Get (0));
  tchPfifo.Install (devices_sink);

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bottleneckBandwidth));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));
  bottleneckLink.SetQueue ("ns3::DropTailQueue");

  NetDeviceContainer devices_gateway;
  devices_gateway = bottleneckLink.Install (gateway.Get (0), gateway.Get (1));
  // only backbone link has Pi Square queue disc
  QueueDiscContainer queueDiscs = tchPi.Install (devices_gateway);

  NS_LOG_INFO ("Assign IP Addresses");
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  // Configure the source and sink net devices
  // and the channels between the source/sink and the gateway
  // Ipv4InterfaceContainer sink_Interfaces;
  Ipv4InterfaceContainer interfaces[50];
  Ipv4InterfaceContainer interfaces_sink;
  Ipv4InterfaceContainer interfaces_gateway;

  for (i = 0; i < 50; i++)
    {
      address.NewNetwork ();
      interfaces[i] = address.Assign (devices[i]);
    }

  address.NewNetwork ();
  interfaces_gateway = address.Assign (devices_gateway);

  address.NewNetwork ();
  interfaces_sink = address.Assign (devices_sink);

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  // Configure application
  AddressValue remoteAddress (InetSocketAddress (interfaces_sink.GetAddress (1), port));
  for (uint16_t i = 0; i < source.GetN (); i++)
    {
      Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));
      BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
      ftp.SetAttribute ("Remote", remoteAddress);
      ftp.SetAttribute ("SendSize", UintegerValue (1000));

      ApplicationContainer sourceApp = ftp.Install (source.Get (i));
      sourceApp.Start (Seconds (0));
      sourceApp.Stop (Seconds (stopTime - 1));

      sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = sinkHelper.Install (sink);
      sinkApp.Start (Seconds (0));
      sinkApp.Stop (Seconds (stopTime));
    }

  if (writeForPlot)
    {
      filePlotQueueDel << pathOut << "/" << "pi-square-second-delay.plotme";
      remove (filePlotQueueDel.str ().c_str ());

      Ptr<QueueDisc> queue = queueDiscs.Get (0);
      Simulator::ScheduleNow (&CheckQueueDel, queue);
    }

  if (isPcapEnabled)
    {
      bottleneckLink.EnablePcap (pcapFileName,gateway,false);
    }

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> allMon;
  allMon = flowmon.InstallAll ();
  flowmon.SerializeToXmlFile ("pi-square-second.xml", true, true);

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  if (printPiStats)
    {
      PiSquareQueueDisc::Stats st = StaticCast<PiSquareQueueDisc> (queueDiscs.Get (0))->GetStats ();
      std::cout << "*** PI Square stats from Node 50 queue ***" << std::endl;
      std::cout << "\t " << st.unforcedDrop << " drops due to probability " << std::endl;
      std::cout << "\t " << st.forcedDrop << " drops due queue full" << std::endl;

      st = StaticCast<PiSquareQueueDisc> (queueDiscs.Get (1))->GetStats ();
      std::cout << "*** PI Square stats from Node 51 queue ***" << std::endl;
      std::cout << "\t " << st.unforcedDrop << " drops due to probability " << std::endl;
      std::cout << "\t " << st.forcedDrop << " drops due queue full" << std::endl;
    }

  Simulator::Destroy ();
  return 0;
}
