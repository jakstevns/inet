#include <iomanip> // for setprecision

#include "ns3/drop-tail-queue.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

std::vector<std::pair<double, uint32_t>> cwndOverTime;

void
CwndChange (std::string context, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (context << " at time " << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  double simTime = ns3::Simulator::Now ().GetSeconds ();
  cwndOverTime.push_back(std::make_pair(simTime, newCwnd));
}

void
TraceCwnd(uint32_t nodeId)
{
    Config::Connect("/NodeList/" + std::to_string(nodeId) +
                        "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                    MakeCallback(&CwndChange));
}

void
RxBegin (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("Reception start at " << std::setprecision(15) << Simulator::Now ().GetSeconds ());
}

void
RxEnd (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("Reception end at " << std::setprecision(15) << Simulator::Now ().GetSeconds ());
}

void
TxBegin (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("Transmission start at " << std::setprecision(15) << Simulator::Now ().GetSeconds ());
}

void
TxEnd (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("Transmission end at " << std::setprecision(15) << Simulator::Now ().GetSeconds ());
}

void
PacketDropTrace (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("Packet drop at time " << std::setprecision(15) << Simulator::Now ().GetSeconds ());
}

// Global variable to hold time-queue length pairs
std::vector<std::pair<double, uint32_t>> queueLengthOverTime;

void
TcQueueTrace (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_UNCOND ("Queue size from " << oldValue << " to " << newValue << " at time " << std::setprecision(15) << Simulator::Now ().GetSeconds ());
  double simTime = ns3::Simulator::Now ().GetSeconds ();
  queueLengthOverTime.push_back(std::make_pair(simTime, newValue));
}

int main(int argc, char *argv[]) {
//  LogComponentEnableAll(LOG_LEVEL_INFO);

  // Create Nodes
  NodeContainer nodes;
  nodes.Create(3);

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));


  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));
  Config::SetDefault ("ns3::TcpSocketBase::Timestamp", BooleanValue (false));
//  Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (536));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  Config::SetDefault("ns3::TcpL4Protocol::RecoveryType", TypeIdValue(TypeId::LookupByName("ns3::TcpClassicRecovery")));

  // Point-to-Point setup
  PointToPointHelper p2p1, p2p2;
  p2p1.SetDeviceAttribute("DataRate", StringValue("101Mbps"));
  p2p1.SetChannelAttribute ("Delay", StringValue ("2ms"));
  p2p1.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("10p")));
  p2p2.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p2.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer dev1, dev2;
  dev1 = p2p1.Install(nodes.Get(0), nodes.Get(1));
  dev2 = p2p2.Install(nodes.Get(1), nodes.Get(2));

  Ptr<PointToPointNetDevice> p2pDevice;
  Ptr<Queue<Packet>> queue;

  // For Router and Node0 connection
  p2pDevice = DynamicCast<PointToPointNetDevice>(dev1.Get (1));
  queue = CreateObjectWithAttributes<DropTailQueue<Packet>>("MaxSize", QueueSizeValue(QueueSize("10p")));
  p2pDevice->SetQueue(queue);

  // For Router and Node1 connection
  p2pDevice = DynamicCast<PointToPointNetDevice>(dev2.Get (0));
  queue = CreateObjectWithAttributes<DropTailQueue<Packet>>("MaxSize", QueueSizeValue(QueueSize("10p")));
  queue->TraceConnectWithoutContext ("PacketsInQueue", MakeCallback (&TcQueueTrace));
  queue->TraceConnectWithoutContext ("Drop", MakeCallback (&PacketDropTrace));
  p2pDevice->SetQueue(queue);

  dev2.Get (0)->TraceConnectWithoutContext ("PhyTxBegin", MakeCallback (&TxBegin));
  dev2.Get (0)->TraceConnectWithoutContext ("PhyTxEnd", MakeCallback (&TxEnd));

  dev1.Get (1)->TraceConnectWithoutContext ("PhyRxBegin", MakeCallback (&RxBegin));
  dev1.Get (1)->TraceConnectWithoutContext ("PhyRxEnd", MakeCallback (&RxEnd));

  // Internet stack
  InternetStackHelper internet;
  internet.Install(nodes);

  // IP address setup
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface1 = ipv4.Assign(dev1);

  ipv4.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iface2 = ipv4.Assign(dev2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  uint16_t remotePort = 1000;

  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (iface2.GetAddress (1), remotePort));
  source.SetAttribute ("MaxBytes", UintegerValue (1000000));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (10.0));

  Address sinkAddress(InetSocketAddress(iface2.GetAddress(1), remotePort));
  PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
  ApplicationContainer sinkApps = sinkHelper.Install(nodes.Get(2));
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(10.0));

  Simulator::Schedule(MilliSeconds(1), &TraceCwnd, 0);

  // Enable Pcap tracing
  p2p1.EnablePcapAll("example2");

  // Run simulation
  Simulator::Run();
  Simulator::Destroy();

  // Writing to a file
  std::ofstream ofs1 ("ns3-queue-length.dat", std::ios::out);
  for (auto &x : queueLengthOverTime)
    ofs1 << x.first << " " << x.second << std::endl;
  ofs1.close ();

  std::ofstream ofs2 ("ns3-cwnd.dat", std::ios::out);
  for (auto &x : cwndOverTime)
    ofs2 << x.first << " " << x.second << std::endl;
  ofs2.close ();

  return 0;
}

