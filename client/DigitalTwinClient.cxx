//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/codes.h"
#include <mpi.h>
#include <ross.h>

#include <codes/GlobalDefines.h>
#include <codes/lp-io.h>
#include <codes/models/pdes/hosts/SimpleServer.h>
#include <codes/orchestrator/Orchestrator.h>
#include <codes/util/CodesUtils.h>

#include <memory>

char config_file[1024] = "\0";
static int do_lp_io = 0;
static char lp_io_dir[256] = { '\0' };
static lp_io_handle io_handle;
static unsigned int lp_io_use_suffix = 0;

const tw_optdef AppOptions[] = { TWOPT_GROUP("CODES Digital Twin"),
  TWOPT_CHAR("config-file", config_file, "path to configuration file"),
  TWOPT_CHAR("lp-io-dir", lp_io_dir, "Where to place io output (unspecified -> no output"),
  TWOPT_UINT("lp-io-use-suffix", lp_io_use_suffix,
    "Whether to append uniq suffix to lp-io directory (default 0)"),
  TWOPT_END() };

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
  MPI_Comm_rank(MPI_COMM_CODES, &rank);
  MPI_Comm_size(MPI_COMM_CODES, &nprocs);

  // setup the orchestrator and pass the config file
  auto& orchestrator = codes::Orchestrator::GetInstance();

  orchestrator.ConfigureSimulation(config_file);

  // TODO: add this in
  // model_net_enable_sampling(sampling_interval, sampling_end_time);
  if (lp_io_dir[0])
  {
    do_lp_io = 1;
    int flags = lp_io_use_suffix ? LP_IO_UNIQ_SUFFIX : 0;
    int ret = lp_io_prepare(lp_io_dir, flags, &io_handle, MPI_COMM_CODES);
    assert(ret == 0 || !"lp_io_prepare failure");
  }

  tw_run();

  orchestrator.ReportModelNetStats();

  if (do_lp_io)
  {
    int ret = lp_io_flush(io_handle, MPI_COMM_CODES);
    assert(ret == 0 || !"lp_io_flush failure");
  }

  tw_end();

  return EXIT_SUCCESS;
}
