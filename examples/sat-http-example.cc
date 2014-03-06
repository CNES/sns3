/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/satellite-module.h>
#include <ns3/traffic-module.h>
#include <ns3/config-store-module.h>

using namespace ns3;


/**
 * \ingroup satellite
 * \brief Example of using HTTP traffic model in a satellite network.
 *
 * One HTTP server application is installed in the first GW user. Then one HTTP
 * client application is installed in each UT user, configured to point to the
 * server. TCP protocol is used between the applications.
 *
 * By default, the SIMPLE test scenario is used. Another test scenario can be
 * given from command line as user argument, e.g.:
 *
 *     $ ./waf --run="sat-http-example --scenario=larger"
 *     $ ./waf --run="sat-http-example --scenario=full"
 *
 * Simulation runs for 1000 seconds by default. This can be changed from the
 * command line argument as well, e.g.:
 *
 *     $ ./waf --run="sat-http-example --duration=500"
 *
 * To see help for user arguments:
 *
 *     $ ./waf --run "sat-http-example --PrintHelp"
 *
 */
NS_LOG_COMPONENT_DEFINE ("sat-http-example");


int
main (int argc, char *argv[])
{
  std::string scenario = "simple";
  double duration = 1000;
  std::string scenarioLogFile = "";
  SatHelper::PreDefinedScenario_t satScenario = SatHelper::SIMPLE;

  // read command line parameters given by user
  CommandLine cmd;
  cmd.AddValue ("scenario", "Test scenario to use. (simple, larger or full)",
                scenario);
  cmd.AddValue ("duration", "Simulation duration (in seconds)",
                duration);
  cmd.AddValue ("logFile", "File name for scenario creation log",
                scenarioLogFile);
  cmd.Parse (argc, argv);

  if (scenario == "larger")
    {
      satScenario = SatHelper::LARGER;
    }
  else if (scenario == "full")
    {
      satScenario = SatHelper::FULL;
    }

  //LogComponentEnableAll (LOG_PREFIX_ALL);
  //LogComponentEnable ("HttpClient", LOG_LEVEL_ALL);
  //LogComponentEnable ("HttpServer", LOG_LEVEL_ALL);
  LogComponentEnable ("sat-http-example", LOG_LEVEL_INFO);

  // remove next line from comments to run real time simulation
  // GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

  Config::SetDefault ("ns3::SatUserHelper::BackboneNetworkType",
                      EnumValue (SatUserHelper::NETWORK_TYPE_CSMA));

  // Create reference system, two options:
  // - "Scenario72"
  // - "Scenario98"
  std::string scenarioName = "Scenario72";
  // std::string scenarioName = "Scenario98";

  Ptr<SatHelper> helper = CreateObject<SatHelper> (scenarioName);

  if (scenarioLogFile != "")
    {
      helper->EnableCreationTraces (scenarioLogFile, false);
    }

  helper->CreateScenario (satScenario);

  // get users
  NodeContainer utUsers = helper->GetUtUsers ();
  NodeContainer gwUsers = helper->GetGwUsers ();

  HttpHelper httpHelper ("ns3::TcpSocketFactory");
  httpHelper.InstallUsingIpv4 (gwUsers.Get (0), utUsers);
  httpHelper.GetServer ().Start (Seconds (1.0));
  httpHelper.GetClients ().Start (Seconds (3.0));

  // install KPI statistics collector
  HttpKpiHelper kpiHelper (&httpHelper);

  NS_LOG_INFO ("--- sat-http-example ---");
  NS_LOG_INFO ("  Scenario used: " << scenario);
  NS_LOG_INFO ("  Creation logFile: " << scenarioLogFile);
  NS_LOG_INFO ("  ");

  //LogComponentEnable ("SatStatsHelper", LOG_PREFIX_ALL);
  //LogComponentEnable ("SatStatsHelper", LOG_LEVEL_ALL);
  //LogComponentEnable ("SatStatsFwdThroughputHelper", LOG_PREFIX_ALL);
  //LogComponentEnable ("SatStatsFwdThroughputHelper", LOG_LEVEL_ALL);
  //LogComponentEnable ("SatStatsRtnThroughputHelper", LOG_PREFIX_ALL);
  //LogComponentEnable ("SatStatsRtnThroughputHelper", LOG_LEVEL_ALL);
  Ptr<SatStatsHelperContainer> stats = CreateObject<SatStatsHelperContainer> (helper);

  stats->AddGlobalFwdThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddGlobalFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddGlobalFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerGwFwdThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerGwFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerGwFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerBeamFwdThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerBeamFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerBeamFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerUtFwdThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerUtFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerUtFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerUtUserFwdThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerUtUserFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerUtUserFwdThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);

  stats->AddGlobalRtnThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddGlobalRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddGlobalRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerGwRtnThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerGwRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerGwRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerBeamRtnThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerBeamRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerBeamRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerUtRtnThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerUtRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerUtRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
  stats->AddPerUtUserRtnThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
  stats->AddPerUtUserRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
  stats->AddPerUtUserRtnThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);

  // Output config store to txt format.
  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig;
  outputConfig.ConfigureAttributes ();

  Simulator::Stop (Seconds (duration));
  Simulator::Run ();

  kpiHelper.Print ();

  Simulator::Destroy ();

  return 0;

} // end of `int main (int argc, char *argv[])`
