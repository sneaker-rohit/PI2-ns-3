/*
 * This script simulates a mix of TCP and UDP traffic for PI Square evaluation
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

NS_LOG_COMPONENT_DEFINE ("PiSquareThird");

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
  std::string pcapFileName = "pi-square-third.pcap";

  float stopTime = startTime + simDuration;

  CommandLine cmd;
  cmd.Parse (argc,argv);

  LogComponentEnable ("PiSquareQueueDisc", LOG_LEVEL_INFO);

  std::string bottleneckBandwidth = "10Mbps";
  std::string bottleneckDelay = "38ms";

  std::string accessBandwidth = "10Mbps";
  std::string accessDelay = "1ms";

  NodeContainer source;
  source.Create (5);

  NodeContainer udpsource;
  udpsource.Create (2);

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
  accessLink.SetDeviceAttribute ("DataRate", StringValue (accessBandwidth));
  accessLink.SetChannelAttribute ("Delay", StringValue (accessDelay));

  NetDeviceContainer devices[5];
  for (i = 0; i < 5; i++)
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
  Ipv4InterfaceContainer interfaces[5];
  Ipv4InterfaceContainer interfaces_sink;
  Ipv4InterfaceContainer interfaces_gateway;
  Ipv4InterfaceContainer udpinterfaces[2];

  NetDeviceContainer udpdevices[2];

  for (i = 0; i < 5; i++)
    {
      address.NewNetwork ();
      interfaces[i] = address.Assign (devices[i]);
    }

  for (i = 0; i < 2; i++)
    {
      udpdevices[i] = accessLink.Install (udpsource.Get (i), gateway.Get (0));
      address.NewNetwork ();
      udpinterfaces[i] = address.Assign (udpdevices[i]);
    }

  address.NewNetwork ();
  interfaces_gateway = address.Assign (devices_gateway);

  address.NewNetwork ();
  interfaces_sink = address.Assign (devices_sink);

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;
  uint16_t port1 = 50001;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  Address sinkLocalAddress1 (InetSocketAddress (Ipv4Address::GetAny (), port1));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  PacketSinkHelper sinkHelper1 ("ns3::UdpSocketFactory", sinkLocalAddress1);

  // Configure application
  AddressValue remoteAddress (InetSocketAddress (interfaces_sink.GetAddress (1), port));
  AddressValue remoteAddress1 (InetSocketAddress (interfaces_sink.GetAddress (1), port1));
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

  OnOffHelper clientHelper6 ("ns3::UdpSocketFactory", Address ());
  clientHelper6.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper6.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper6.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
  clientHelper6.SetAttribute ("PacketSize", UintegerValue (1000));

  ApplicationContainer clientApps6;

  clientHelper6.SetAttribute ("Remote", remoteAddress1);
  clientApps6.Add (clientHelper6.Install (udpsource.Get (0)));
  clientApps6.Start (Seconds (0));
  clientApps6.Stop (Seconds (stopTime - 1));

  OnOffHelper clientHelper7 ("ns3::UdpSocketFactory", Address ());
  clientHelper7.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper7.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper7.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
  clientHelper7.SetAttribute ("PacketSize", UintegerValue (1000));

  ApplicationContainer clientApps7;
  clientHelper7.SetAttribute ("Remote", remoteAddress1);
  clientApps7.Add (clientHelper7.Install (udpsource.Get (1)));
  clientApps7.Start (Seconds (0));
  clientApps7.Stop (Seconds (stopTime - 1));

  sinkHelper1.SetAttribute ("Protocol", TypeIdValue (UdpSocketFactory::GetTypeId ()));
  ApplicationContainer sinkApp1 = sinkHelper1.Install (sink);
  sinkApp1.Start (Seconds (0));
  sinkApp1.Stop (Seconds (stopTime));

  if (writeForPlot)
    {
      filePlotQueueDel << pathOut << "/" << "pi-square-third-delay.plotme";
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
  flowmon.SerializeToXmlFile ("pi-square-third.xml", true, true);

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  if (printPiStats)
    {
      PiSquareQueueDisc::Stats st1 = StaticCast<PiSquareQueueDisc> (queueDiscs.Get (0))->GetStats ();
      std::cout << "*** PI Square stats from Node 7 queue ***" << std::endl;
      std::cout << "\t " << st1.unforcedDrop << " drops due to probability " << std::endl;
      std::cout << "\t " << st1.forcedDrop << " drops due queue full" << std::endl;

      PiSquareQueueDisc::Stats st2 = StaticCast<PiSquareQueueDisc> (queueDiscs.Get (1))->GetStats ();
      std::cout << "*** PI Square stats from Node 8 queue ***" << std::endl;
      std::cout << "\t " << st2.unforcedDrop << " drops due to probability " << std::endl;
      std::cout << "\t " << st2.forcedDrop << " drops due queue full" << std::endl;

    }

  Simulator::Destroy ();
  return 0;
}
