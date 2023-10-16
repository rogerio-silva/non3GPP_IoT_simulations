#include "ns3/end-device-lora-phy.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/lorawan-module.h"
#include "ns3/propagation-module.h"
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <unistd.h>

#define ARRAY_SIZE 8

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("GatewaysDensityOrientedPlacement");

struct pos
{
  int v; // Value
  double p; // Percentual
  int w; // pos x
  int h; // pos y
};

int xFac = 1; // 4 ** xFac = número de quadrantes

int arrayDevices[ARRAY_SIZE][ARRAY_SIZE];
int arrayGateways[ARRAY_SIZE][ARRAY_SIZE];

bool
pos_compare (const pos &p1, const pos &p2)
{
  return p1.v > p2.v;
}

int
sumPos (std::vector<pos> p, int l)
{
  int s = 0;
  for (int i = 0; i < l; i++)
    s += p[i].v;
  return s;
}

int
sumElements (int i, int j, int qtdI, int qtdJ)
{
  int sumE = 0;
  for (int x = i; x < i + qtdI; ++x)
    {
      for (int y = j; y < j + qtdJ; ++y)
        {
          sumE += arrayDevices[x][y];
        }
    }
  return sumE;
}

int
main (int argc, char *argv[])
{
  int nGateways = 0;
  int nDevices = 0;
  int seed = 1;
  bool debug = false;
  //  bool debug = false;

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("nGateways", "Number of gateways to include in the simulation", nGateways);
  cmd.AddValue ("seed", "Independent replications seed", seed);
  cmd.Parse (argc, argv);

  ns3::RngSeedManager::SetSeed (seed);

  //  LogComponentEnable ("DevicesDensityOrientedPlacement", LOG_LEVEL_ALL);
  //  LogComponentEnableAll (LOG_PREFIX_FUNC);
  //  LogComponentEnableAll (LOG_PREFIX_NODE);
  //  LogComponentEnableAll (LOG_PREFIX_TIME);
  //  LogComponentEnableAll (LOG_ERROR);

  NS_LOG_INFO ("Reading devices positions...");
  double edX = 0.0, edY = 0.0, edZ = 0.0;

  std::string cwd = get_current_dir_name();
  std::string path = cwd + "/data/placement/";
  std::string filename = path + 
      "endDevices_LNM_Placement_" +
      std::to_string (seed) + "s+" + std::to_string (nDevices) + "d.dat";

  const char *c = filename.c_str ();
  // Get Devices position from File
  std::ifstream in_File (c);

  std::fill (arrayDevices[0], arrayDevices[0] + xFac * xFac, 0);
  std::fill (arrayGateways[0], arrayGateways[0] + xFac * xFac, 0);
  int x, y;
  if (!in_File)
    {
      std::cout << "Could not open the file - '" << filename << "'" << std::endl;
    }
  else
    {
      while (in_File >> edX >> edY >> edZ)
        {
          x = floor (edX / (10000 / ARRAY_SIZE));
          y = floor (edY / (10000 / ARRAY_SIZE));
          arrayDevices[x][y]++;
        }
      in_File.close ();
    }

  // Count left and right side elements
  int sumE = 0;

  std::vector<pos> vecPosD_2;
  for (int k = 0; k < ARRAY_SIZE; k += 4)
    {
      for (int l = 0; l < ARRAY_SIZE; l += 4)
        {
          sumE = sumElements (k, l, 4, 8);
          if (sumE > 0) // if it has devices on the quadrant
            vecPosD_2.push_back ({sumE, sumE / (float) nDevices, k, l});
        }
    }

  // Count elements of quadrants
  std::vector<pos> vecPosD_4;
  int gatInstalled[4] = {0, 0, 0, 0};
  int gatToInstall[4] = {0, 0, 0, 0};
  int iQuad;
  int sumToInstall = 0;
  for (int k = 0; k < ARRAY_SIZE; k += 4)
    {
      for (int l = 0; l < ARRAY_SIZE; l += 4)
        {
          sumE = sumElements (k, l, 4, 4);
          if (sumE > 0) // if it has devices on the quadrant
            vecPosD_4.push_back ({sumE, sumE / (float) nDevices, k, l});
        }
    }

  // Install gateways
  std::string fileNS3 = path + "/densityOrientedPlacement_" + std::to_string (seed) + "s+" +
                        std::to_string (nDevices) + "d+" + std::to_string (nGateways) + "g.dat";
  const char *cN = fileNS3.c_str ();

  std::ofstream devicesNS3File;
  devicesNS3File.open (cN);

  // Sort arrays (densest first)
  std::sort (vecPosD_2.begin (), vecPosD_2.end (), pos_compare);
  std::sort (vecPosD_4.begin (), vecPosD_4.end (), pos_compare);

  for (int i = 0; i < (int) vecPosD_4.size (); ++i)
    {
      iQuad = ((vecPosD_4[i].w < 4 && vecPosD_4[i].h < 4)
                   ? 0
                   : ((vecPosD_4[i].w >= 4 && vecPosD_4[i].h >= 4)
                          ? 3
                          : ((vecPosD_4[i].w < 4 && vecPosD_4[i].h >= 4) ? 2 : (1))));
      gatToInstall[iQuad] = (round (nGateways * vecPosD_4[i].p) + sumToInstall) > nGateways
                                ? nGateways - sumToInstall
                                : round (nGateways * vecPosD_4[i].p);
      if (vecPosD_4[i].p > 0 && gatToInstall[iQuad] == 0 && sumToInstall < nGateways)
        gatToInstall[iQuad]++;
      sumToInstall += gatToInstall[iQuad];
    }

  double gatewayAltitude = 30.0;
  int dMin = 3;
  while (dMin > 0)
    {
      for (int i1 = 0; i1 < (int) vecPosD_4.size (); i1++)
        {
          std::vector<pos> vecPosD_16;
          iQuad = ((vecPosD_4[i1].w < 4 && vecPosD_4[i1].h < 4)
                       ? 0
                       : ((vecPosD_4[i1].w >= 4 && vecPosD_4[i1].h >= 4)
                              ? 3
                              : ((vecPosD_4[i1].w < 4 && vecPosD_4[i1].h >= 4) ? 2 : (1))));
          if (gatToInstall[iQuad] > 0)
            {
              for (int k = vecPosD_4[i1].w; k < vecPosD_4[i1].w + 4; k += 2)
                {
                  for (int l = vecPosD_4[i1].h; l < vecPosD_4[i1].h + 4; l += 2)
                    {
                      sumE = sumElements (k, l, 2, 2);
                      if (sumE > 0) // if it has devices on the quadrant
                        vecPosD_16.push_back ({sumE, sumE / (float) nDevices, k, l});
                    }
                }
              std::sort (vecPosD_16.begin (), vecPosD_16.end (), pos_compare);
              for (int i2 = 0; i2 < (int) vecPosD_16.size (); ++i2)
                {
                  std::vector<pos> vecPosD_64;
                  for (int k = vecPosD_16[i2].w; k < vecPosD_16[i2].w + 2; k += 1)
                    {
                      for (int l = vecPosD_16[i2].h; l < vecPosD_16[i2].h + 2; l += 1)
                        {
                          sumE = arrayDevices[k][l];
                          if (sumE > 0) // if it has devices on the quadrant
                            vecPosD_64.push_back ({sumE, sumE / (float) nDevices, k, l});
                        }
                    }
                  std::sort (vecPosD_64.begin (), vecPosD_64.end (), pos_compare);
                  for (int i3 = 0; i3 < (int) vecPosD_64.size (); ++i3)
                    {
                      if (vecPosD_64[i3].v > dMin && gatInstalled[iQuad] < gatToInstall[iQuad])
                        {
                          if (!arrayGateways[vecPosD_64[i3].w][vecPosD_64[i3].h])
                            {
                              devicesNS3File << vecPosD_64[i3].w * 1250 + 625 << " "
                                             << vecPosD_64[i3].h * 1250 + 625 << " "
                                             << gatewayAltitude << std::endl;
                              arrayGateways[vecPosD_64[i3].w][vecPosD_64[i3].h] = 1;
                              if(debug)
                                std::cout << vecPosD_64[i3].w * 1250 + 625 << " "
                                        << vecPosD_64[i3].h * 1250 + 625 << " " << gatewayAltitude
                                        << std::endl;

                              gatInstalled[iQuad]++;
                              gatToInstall[iQuad]--;
                            }
                        }
                    }
                }
            }
        }
      dMin--;
    }
  //  std::cout << gatInstalled;
  // Install gateways left
  for (int i1 = 0; i1 < (int) vecPosD_4.size (); i1++)
    {
      iQuad = ((vecPosD_4[i1].w < 4 && vecPosD_4[i1].h < 4)
                   ? 0
                   : ((vecPosD_4[i1].w >= 4 && vecPosD_4[i1].h >= 4)
                          ? 3
                          : ((vecPosD_4[i1].w < 4 && vecPosD_4[i1].h >= 4) ? 2 : (1))));
      if (gatInstalled[iQuad] < gatToInstall[iQuad])
        {
          std::vector<pos> vecPosD_16;
          for (int k = vecPosD_4[i1].w; k < vecPosD_4[i1].w + 4; k += 2)
            {
              for (int l = vecPosD_4[i1].h; l < vecPosD_4[i1].h + 4; l += 2)
                {
                  sumE = sumElements (k, l, 2, 2);
                  if (sumE > 0) // if it has devices on the quadrant
                    vecPosD_16.push_back ({sumE, sumE / (float) nDevices, k, l});
                }
            }
          std::sort (vecPosD_16.begin (), vecPosD_16.end (), pos_compare);
          for (int i2 = 0; i2 < (int) vecPosD_16.size (); ++i2)
            {
              std::vector<pos> vecPosD_64;
              //          gatToInstall = floor(vecPosD_16[i1].p * nGateways);
              for (int k = vecPosD_16[i2].w; k < vecPosD_16[i2].w + 2; k += 1)
                {
                  for (int l = vecPosD_16[i2].h; l < vecPosD_16[i2].h + 2; l += 1)
                    {
                      sumE = arrayDevices[k][l];
                      if (sumE > 0) // if it has devices on the quadrant
                        vecPosD_64.push_back ({sumE, sumE / (float) nDevices, k, l});
                    }
                }
              std::sort (vecPosD_64.begin (), vecPosD_64.end (), pos_compare);
              for (int i3 = 0; i3 < (int) vecPosD_64.size (); ++i3)
                {
                  if (vecPosD_64[i3].v == 1 && gatInstalled[iQuad] < gatToInstall[iQuad])
                    {
                      if (!arrayGateways[vecPosD_64[i3].w][vecPosD_64[i3].h])
                        {
                          devicesNS3File << vecPosD_64[i3].w * 1250 + 625 << " "
                                         << vecPosD_64[i3].h * 1250 + 625 << " " << gatewayAltitude
                                         << std::endl;
                          arrayGateways[vecPosD_64[i3].w][vecPosD_64[i3].h] = 1;
                          if(debug)
                            std::cout << vecPosD_64[i3].w * 1250 + 625 << " "
                                    << vecPosD_64[i3].h * 1250 + 625 << " " << gatewayAltitude
                                    << std::endl;
                          gatInstalled[iQuad]++;
                          gatToInstall[iQuad]--;
                        }
                    }
                }
            }
        }
    }
  // Still have some gateway uninstalled
  int sumInstalled = 0;
  for (int i = 0; i < 4; ++i)
    sumInstalled += gatInstalled[i];
  sumToInstall = nGateways - sumInstalled;
  std::vector<pos> vecPosD_64;
  for (int i = 0; i < 8; ++i)
    {
      for (int j = 0; j < 8; ++j)
        {
          if (!arrayGateways[i][j] && arrayDevices[i][j])
            {
              sumE = arrayDevices[i][j];
              if (sumE > 0) // if it has devices on the quadrant
                vecPosD_64.push_back ({sumE, sumE / (float) nDevices, i, j});
            }
        }
    }
  std::sort (vecPosD_64.begin (), vecPosD_64.end (), pos_compare);
  for (int i = 0; i < (int) vecPosD_64.size () && sumToInstall > 0; ++i)
    {
      devicesNS3File << vecPosD_64[i].w * 1250 + 625 << " " << vecPosD_64[i].h * 1250 + 625 << " "
                     << gatewayAltitude << std::endl;
      if(debug)
        std::cout << vecPosD_64[i].w * 1250 + 625 << " " << vecPosD_64[i].h * 1250 + 625 << " "
                << gatewayAltitude << std::endl;
      arrayGateways[vecPosD_64[i].w][vecPosD_64[i].h] = 1;
      sumToInstall--;
    }

  devicesNS3File.close ();

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop ();

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
