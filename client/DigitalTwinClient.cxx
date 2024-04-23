//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <mpi.h>
#include <ross.h>

#include <codes/GlobalDefines.h>
#include <codes/config/configuration.h>
#include <codes/model-net/lp-io.h>
#include <codes/models/pdes/hosts/SimpleServer.h>
#include <codes/orchestrator/Orchestrator.h>
#include <codes/util/CodesUtils.h>

#include <iostream>
#include <memory>

char config_file[1024] = "\0";

const tw_optdef AppOptions[] = { TWOPT_GROUP("CODES Digital Twin"),
  TWOPT_CHAR("config-file", config_file, "path to configuration file"), TWOPT_END() };

// For now this is just some code hacked together for testing while
// implementing other functionality. I'd like for the user to have a single
// executable they need to run and they're just creating their configuration file
// (or at most adding new models), instead of needing to write their own driver
// code for every new type of simulation they want to do
int main(int argc, char* argv[])
{
  UseYAMLConfig = 1;
  tw_opt_add(AppOptions);
  tw_init(&argc, &argv);

  g_tw_ts_end = codes::SecondsToNS(60 * 60 * 24 * 365); /* one year, in nsecs */

  int rank, nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  std::string yamlFile = config_file;
  // setup the orchestrator and pass the config file
  auto& orchestrator = codes::orchestrator::Orchestrator::GetInstance();

  orchestrator.ParseConfig(config_file);
  // this part will be removed once orchestrator is working
  // configuration_load(argv[2], MPI_COMM_WORLD, &config);

  // need to figure out how to add the correct LP types
  // though this will be done in the orchestrator
  SimpleServerAddLPType();

  // registers all model-net lps in ross. should be called after configuring, but
  // before codes_mapping_setup()
  // for this I need to add the lps to the NETWORK_DEF in model-net.h
  // also this relies on a global variable lpconf from configuration.h
  // so it's heavliy dependent on that.
  // will need to rewrite this to work on my config
  model_net_register_yaml();

  // says this loads the config file and sets up the number of LPs on each PE
  // this is similar to the above, in that its dependent on lpconf
  // these two functions will be replaced by functionality in the orchestrator
  // codes_mapping_setup_yaml();
  auto mapper = orchestrator.GetMapper();
  mapper->MappingSetup();

  int num_nets;
  int* net_ids;
  // similar here, dependent on old configuration stuff
  // configures all model-net lps and returns the set of network ids
  net_ids = model_net_configure_yaml(&num_nets);
  // net_ids = orchestrator.ModelNetConfigure(num_nets);
  assert(num_nets == 1);
  int net_id = net_ids[0];
  SimpleServerSetNetId(net_id);

  // also depends on config globals
  // int num_servers = codes_mapping_get_lp_count_yaml("SimpleServer");
  // assert(num_servers == 3);

  lp_io_handle handle;
  char name[15] = "modelnet-test\0";
  if (lp_io_prepare(name, LP_IO_UNIQ_SUFFIX, &handle, MPI_COMM_WORLD) < 0)
  {
    return EXIT_FAILURE;
  }

  tw_run();
  model_net_report_stats(net_id);

  if (lp_io_flush(handle, MPI_COMM_WORLD) < 0)
  {
    return (-1);
  }

  tw_end();

  return EXIT_SUCCESS;
}
