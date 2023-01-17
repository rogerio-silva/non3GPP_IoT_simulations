/*
 * This script simulates a simple network in which one end device sends one
 * packet to the gateway.
 */
#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/one-shot-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/network-server-helper.h"
#include "ns3/forwarder-helper.h"
#include "ns3/lorawan-module.h"
#include "ns3/propagation-module.h"
#include <algorithm>
#include <ctime>
#include <iomanip>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("EquidistantExperiment");

NodeContainer endDevices;
NodeContainer gateways;
Ptr<LoraChannel> channel;
MobilityHelper mobilityED, mobilityGW;

int nDevices = 0;
int nGateways = 0;

int noMoreReceivers = 0;
int interfered = 0;
int received = 0;
int underSensitivity = 0;

/**********************
 *  Global Callbacks  *
 **********************/

enum PacketOutcome { _RECEIVED, _INTERFERED, _NO_MORE_RECEIVERS, _UNDER_SENSITIVITY, _UNSET };

struct myPacketStatus
{
  Ptr<Packet const> packet;
  uint32_t senderId;
  uint32_t receiverId;
  Time sentTime;
  Time receivedTime;
  int outcomeNumber;
  std::vector<enum PacketOutcome> outcomes;
};

std::map<Ptr<Packet const>, myPacketStatus> packetTracker;

void
CheckReceptionByAllGWsComplete (std::map<Ptr<Packet const>, myPacketStatus>::iterator it)
{
  // Check whether this packet is received by all gateways
  if ((*it).second.outcomeNumber == nGateways)
    {
      // Update the statistics
      myPacketStatus status = (*it).second;
      for (int j = 0; j < nGateways; j++)
        {
          switch (status.outcomes.at (j))
            {
              case _RECEIVED: {
                received += 1;
                break;
              }
              case _UNDER_SENSITIVITY: {
                underSensitivity += 1;
                break;
              }
              case _NO_MORE_RECEIVERS: {
                noMoreReceivers += 1;
                break;
              }
              case _INTERFERED: {
                interfered += 1;
                break;
              }
              case _UNSET: {
                break;
              }
            }
        }
      // Remove the packet from the tracker
      //      packetTracker.erase (it);
    }
}

void
TransmissionCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  NS_LOG_INFO ("Transmitted a packet from device " << systemId);
  // Create a packetStatus
  myPacketStatus status;
  status.packet = packet;
  status.senderId = systemId;
  status.sentTime = Simulator::Now ();
  status.outcomeNumber = 0;
  status.outcomes = std::vector<enum PacketOutcome> (nGateways, _UNSET);

  packetTracker.insert (std::pair<Ptr<Packet const>, myPacketStatus> (packet, status));
}

void
PacketReceptionCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  // Remove the successfully received packet from the list of sent ones
  NS_LOG_INFO ("A packet was successfully received at gateway " << systemId);

  std::map<Ptr<Packet const>, myPacketStatus>::iterator it = packetTracker.find (packet);
  if ((*it).second.outcomes.size () > systemId - nDevices)
    {
      (*it).second.outcomes.at (systemId - nDevices) = _RECEIVED;
      (*it).second.outcomeNumber += 1;
      //      if ((*it).second.outcomeNumber == 1 || (*it).second.receivedTime == Seconds (0))
      if ((*it).second.receivedTime == Seconds (0))
        {
          (*it).second.receivedTime = Simulator::Now ();
          (*it).second.receiverId = systemId;
        }
      CheckReceptionByAllGWsComplete (it);
    }
}

void
InterferenceCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  // NS_LOG_INFO ("A packet was lost because of interference at gateway " << systemId);

  std::map<Ptr<Packet const>, myPacketStatus>::iterator it = packetTracker.find (packet);
  if ((*it).second.outcomes.size () > systemId - nDevices)
    {
      (*it).second.outcomes.at (systemId - nDevices) = _INTERFERED;
      (*it).second.outcomeNumber += 1;
    }
  CheckReceptionByAllGWsComplete (it);
}

void
NoMoreReceiversCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  NS_LOG_INFO ("A packet was lost because there were no more receivers at gateway " << systemId);

  std::map<Ptr<Packet const>, myPacketStatus>::iterator it = packetTracker.find (packet);
  if ((*it).second.outcomes.size () > systemId - nDevices)
    {
      (*it).second.outcomes.at (systemId - nDevices) = _NO_MORE_RECEIVERS;
      (*it).second.outcomeNumber += 1;
    }
  CheckReceptionByAllGWsComplete (it);
}

void
UnderSensitivityCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  NS_LOG_INFO ("A packet arrived at the gateway under sensitivity at gateway " << systemId);

  std::map<Ptr<Packet const>, myPacketStatus>::iterator it = packetTracker.find (packet);
  if ((*it).second.outcomes.size () > systemId - nDevices)
    {
      (*it).second.outcomes.at (systemId - nDevices) = _UNDER_SENSITIVITY;
      (*it).second.outcomeNumber += 1;
    }
  CheckReceptionByAllGWsComplete (it);
}

/**
* Places the end devices according to the allocator object in the input file..
* @param filename: arquivo de entrada
* @return number of devices
**/
void
EndDevicesPlacement (std::string filename)
{
  double edX = 0.0, edY = 0.0, edZ = 0.0;
  int nDev = 0;
  Ptr<ListPositionAllocator> allocatorED = CreateObject<ListPositionAllocator> ();
  const char *c = filename.c_str ();
  // Get Devices position from File
  std::ifstream in_File (c);
  if (!in_File)
    {
      std::cout << "Could not open the file - '" << filename << "'" << std::endl;
    }
  else
    {
      while (in_File >> edX >> edY >> edZ)
        {
          allocatorED->Add (Vector (edX, edY, edZ));
          nDev++;
        }
      in_File.close ();
    }

  endDevices.Create (nDev);
  mobilityED.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityED.SetPositionAllocator (allocatorED);
  mobilityED.Install (endDevices);
}

/**
* Places the gateways according to the allocator object in the input file..
* @param filename: arquivo de entrada
* @return number of gateways
**/
void
GatewaysPlacement (std::string filename)
{
  double gwX = 0.0, gwY = 0.0, gwZ = 0.0;
  Ptr<ListPositionAllocator> allocatorGW = CreateObject<ListPositionAllocator> ();
  const char *c = filename.c_str ();
  // Get Devices position from File
  std::ifstream in_File (c);
  int nGat = 0;
  if (!in_File)
    {
      std::cout << "Could not open the file - '" << filename << "'" << std::endl;
    }
  else
    {
      while (in_File >> gwX >> gwY >> gwZ)
        {
          allocatorGW->Add (Vector (gwX, gwY, gwZ));
          nGat++;
        }
      in_File.close ();
    }
  gateways.Create (nGat);
  mobilityGW.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityGW.SetPositionAllocator (allocatorGW);
  mobilityGW.Install (gateways);
}

void
PrintEndDevicesParameters (std::string filename)
{
  const char *c = filename.c_str ();
  std::ofstream spreadingFactorFile;
  spreadingFactorFile.open (c);
  for (NodeContainer::Iterator ed = endDevices.Begin (); ed != endDevices.End (); ++ed)
    {
      Ptr<Node> oDevice = *ed;
      Ptr<NetDevice> netDevice = oDevice->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();
      int sf = mac->GetSfFromDataRate (mac->GetDataRate ());
      int txPower = mac->GetTransmissionPower ();
      //      spreadingFactorFile << oGateway->GetId () << " " << oDevice->GetId () << " " << distanceFromGW << " " << sf << " "
      //                              << txPower <<  std::endl;
      spreadingFactorFile << oDevice->GetId () << " " << sf << " " << txPower << " "
                          << (sf * (125000 / (pow (2, sf))) * (4 / 5.0)) << std::endl;
      //      std::cout << sf * (125000/(pow(2,sf))) << std::endl;
    }
  spreadingFactorFile.close ();
}

int
main (int argc, char *argv[])
{
  double simulationTime = 1200;
  double appPeriodSeconds = 30;
  bool okumura = false;
  bool printRates = true;
  bool verbose = false;
  int seed = 1;
  bool up = false;
  int packetSize = 41;

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("nGateways", "Number of gateways to include in the simulation", nGateways);
  cmd.AddValue ("okumura", "Uses okumura-hate propagation mode", okumura);
  cmd.AddValue ("verbose", "Whether to print output or not", verbose);
  cmd.AddValue ("printRates", "Whether to print result rates", printRates);
  cmd.AddValue ("seed", "Independent replications seed", seed);
  cmd.AddValue ("up", "Spread Factor UP", up);
  cmd.Parse (argc, argv);

  RngSeedManager::SetSeed (seed + 100);

  Config::SetDefault ("ns3::EndDeviceLorawanMac::DRControl", BooleanValue (true));

  // Set up logging
  if (verbose)
    {
      LogComponentEnable ("EquidistantExperiment", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
      //      LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
      //      LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
      //      LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LorawanMac", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LorawanMacHelper", LOG_LEVEL_ALL);
      //      LogComponentEnable ("EndDeviceLorawanMac", LOG_LEVEL_ALL);
      //      LogComponentEnable ("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
      //      LogComponentEnable ("GatewayLorawanMac", LOG_LEVEL_ALL);
      //      LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
      LogComponentEnable ("PeriodicSenderHelper", LOG_LEVEL_ALL);
      LogComponentEnable ("PeriodicSender", LOG_LEVEL_ALL);
      LogComponentEnable ("LoraPacketTracker", LOG_LEVEL_ALL);
      //      LogComponentEnable ("NetworkServerHelper", LOG_LEVEL_ALL);
      //      LogComponentEnable ("AdrComponent", LOG_LEVEL_ALL);
      LogComponentEnableAll (LOG_PREFIX_FUNC);
      LogComponentEnableAll (LOG_PREFIX_NODE);
      LogComponentEnableAll (LOG_PREFIX_TIME);

    }
  /************************
  *  Create the channel  *
  ************************/

  // Create the lora channel object
  // modelo de propagação (okumura ou logdistance)
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  if (okumura)
    {
      Ptr<OkumuraHataPropagationLossModel> loss = CreateObject<OkumuraHataPropagationLossModel> ();
      channel = CreateObject<LoraChannel> (loss, delay);
    }
  else
    {
      Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
      loss->SetPathLossExponent (3.76);
      loss->SetReference (1, 10.0);
      channel = CreateObject<LoraChannel> (loss, delay);
    }
  /************************
  *  Create the helpers  *
  ************************/

  NS_LOG_INFO ("Setting up helpers...");

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LorawanMacHelper
  LorawanMacHelper macHelper = LorawanMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();
  helper.EnablePacketTracking ();

  /************************
  *  Create End Devices  *
  ************************/

  NS_LOG_INFO ("Creating end devices...");
  // Create a set of nodes
  EndDevicesPlacement ("./datafile"
                       "/devices/placement/endDevices_LNM_Placement_" +
                       std::to_string (seed) + "s+" + std::to_string (nDevices) + "d.dat");

  // Create the LoraNetDevices of the end devices
  uint8_t nwkId = 54;
  uint32_t nwkAddr = 1864;
  Ptr<LoraDeviceAddressGenerator> addrGen =
      CreateObject<LoraDeviceAddressGenerator> (nwkId, nwkAddr);

  // Create the LoraNetDevices of the end devices
  macHelper.SetAddressGenerator (addrGen);
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
    macHelper.SetRegion (LorawanMacHelper::EU);
  helper.Install (phyHelper, macHelper, endDevices);

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      phy->TraceConnectWithoutContext ("StartSending", MakeCallback (&TransmissionCallback));
    }

  /*********************
  *  Create Gateways  *
  *********************/

  NS_LOG_INFO ("Creating gateways...");

  std::string filename = "./datafile/"
                         "equidistant/placement/equidistantPlacement_" +
                         std::to_string (nGateways) + ".dat";
  GatewaysPlacement (filename);

  // Create a net device for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  for (NodeContainer::Iterator g = gateways.Begin (); g != gateways.End (); ++g)
    {
      Ptr<Node> object = *g;
      // Get the device
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      Ptr<GatewayLoraPhy> gwPhy = loraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();
      gwPhy->TraceConnectWithoutContext ("ReceivedPacket", MakeCallback (&PacketReceptionCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseInterference",
                                         MakeCallback (&InterferenceCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseNoMoreReceivers",
                                         MakeCallback (&NoMoreReceiversCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseUnderSensitivity",
                                         MakeCallback (&UnderSensitivityCallback));
    }
  NS_LOG_DEBUG ("Completed configuration");

  /*********************************************
  *  Install applications on the end devices  *
  *********************************************/

  Time appStopTime = Seconds (simulationTime); // ten minutes
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds)); // each 60 seconds
  appHelper.SetPacketSize (packetSize);
  ApplicationContainer appContainer = appHelper.Install (endDevices);

  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);

  if (up)
    {
      macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
    }
  /**************************
   *  Create Network Server  *
   ***************************/

  // Create the NS node
  NodeContainer networkServer;
  networkServer.Create (1);
  //Create the NetworkServerHelper
  NetworkServerHelper nsHelper = NetworkServerHelper ();
  //Create the ForwarderHelper
  ForwarderHelper forHelper = ForwarderHelper ();

  // Create a NS for the network
  nsHelper.SetAdr ("ns3::AdrComponent");
  nsHelper.EnableAdr (true);
  nsHelper.SetEndDevices (endDevices);
  nsHelper.SetGateways (gateways);
  nsHelper.Install (networkServer);

  //Create a forwarder for each gateway
  forHelper.Install (gateways);

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (appStopTime + Minutes (10));

  Simulator::Run ();
  NS_LOG_INFO ("Computing performance metrics...");

  if (printRates)
    {
      /**
       * Print COMM PARAMETERS
       * **/
      std::string par_filename = "./datafile"
                                 "/equidistant/results/parameters/transmissionParameters_" +
                                 std::to_string (seed) + "_" + std::to_string (nGateways) + "x" +
                                 std::to_string (nDevices) + ".dat";
      PrintEndDevicesParameters (par_filename);

      /**
       * Print PACKETS
       * **/
      std::string packs_filename = "./datafile"
                                   "/equidistant/results/packets/transmissionPackets_" +
                                   std::to_string (seed) + "_" + std::to_string (nGateways) + "x" +
                                   std::to_string (nDevices) + ".dat";
      const char *cPK = packs_filename.c_str ();
      std::ofstream filePKT;
      filePKT.open (cPK, std::ios::out);
      for (std::map<Ptr<Packet const>, myPacketStatus>::iterator p = packetTracker.begin ();
           p != packetTracker.end (); ++p)
        {
          filePKT << (*p).second.senderId << " " << (*p).second.receiverId << " "
                  << (*p).second.sentTime.GetSeconds () << " "
                  << (*p).second.receivedTime.GetSeconds () << " "
                  << (*p).second.receivedTime.GetSeconds () - (*p).second.sentTime.GetSeconds ()
                  << std::endl;
        }
      filePKT.close ();
    }

  Simulator::Destroy ();

  if (printRates)
    {
      NS_LOG_INFO ("Computing performance metrics...");
      LoraPacketTracker &tracker = helper.GetPacketTracker ();

      /**
       * Print GLOBAL PACKET DELIVERY
       * **/

      std::string phyPerformanceFile = "./datafile"
                                       "/equidistant/results/data/transmissionData_" +
                                       std::to_string (nGateways) + "x" +
                                       std::to_string (nDevices) + ".dat";
      const char *c = phyPerformanceFile.c_str ();
      std::ofstream file;
      file.open (c, std::ios::app);
      if (!file)
        {
          file.open (c, std::ios::out);
        }
      // Print total of packets [seed sent received]
      file << seed << " "
           << tracker.CountMacPacketsGlobally (Seconds (0), appStopTime + Minutes (10))
           << std::endl;
      file.close ();

      /**
       * Print PACKET DELIVERY PER GATEWAY
       * **/

      std::string phyPerfPerGatewayFile = "./datafile"
                                          "/equidistant/results/dataPerGateway/transmissionDataPerGateway_" +
                                          std::to_string (seed) + "_" + std::to_string (nGateways) +
                                          "x" + std::to_string (nDevices) + ".dat";
      const char *cG = phyPerfPerGatewayFile.c_str ();
      std::ofstream fileG;
      fileG.open (cG, std::ios::out);
      for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
        {
          Ptr<Node> object = *j;
          // exec_number gateway_id totPacketsSent receivedPackets interferedPackets noMoreGwPackets underSensitivityPackets lostBecauseTxPackets
          fileG << seed << " " << object->GetId () << " "
                << tracker.PrintPhyPacketsPerGw (Seconds (0), appStopTime + Minutes (10), object->GetId ())
                << std::endl;
        }
      fileG.close ();
    }
  return 0;
}
