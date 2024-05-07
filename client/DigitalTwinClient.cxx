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
#include <codes/lp-io.h>
#include <codes/models/pdes/hosts/SimpleServer.h>
#include <codes/orchestrator/Orchestrator.h>
#include <codes/util/CodesUtils.h>

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
  tw_opt_add(AppOptions);
  tw_init(&argc, &argv);

  // TODO: make a config option
  g_tw_ts_end = codes::SecondsToNS(60 * 60 * 24 * 365); /* one year, in nsecs */

  int rank, nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  // setup the orchestrator and pass the config file
  auto& orchestrator = codes::Orchestrator::GetInstance();

  orchestrator.ParseConfig(config_file);

  lp_io_handle handle;
  char name[15] = "modelnet-test\0";
  if (lp_io_prepare(name, LP_IO_UNIQ_SUFFIX, &handle, MPI_COMM_WORLD) < 0)
  {
    return EXIT_FAILURE;
  }

  tw_run();

  orchestrator.ReportModelNetStats();

  if (lp_io_flush(handle, MPI_COMM_WORLD) < 0)
  {
    return (-1);
  }

  tw_end();

  return EXIT_SUCCESS;
}
