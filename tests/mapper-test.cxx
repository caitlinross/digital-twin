
#include "codes/model-net/lp-type-lookup.h"
#include <cstdlib>
#include <iostream>

#include <ross.h>

#include <ross-extern.h>

#include <codes/mapping/Mapper.h>
#include <codes/orchestrator/Orchestrator.h>
#include <string>

struct state
{
  int id_global;
  std::string lp_name;
};

static void init(state* ns, tw_lp* lp)
{
  int dummy;
  auto& orchestrator = codes::Orchestrator::GetInstance();
  auto mapper = orchestrator.GetMapper();

  ns->lp_name = mapper->GetLPTypeName(lp->gid);

  // TODO: add in a bunch of checks to get different info from the mapper and then compare it to the
  // correct values
}

tw_lptype a_lp = {
  (init_f)init,
  (pre_run_f)NULL,
  (event_f)NULL,
  (revent_f)NULL,
  (commit_f)NULL,
  (final_f)NULL,
  (map_f)codes::CodesMapping,
  sizeof(state),
};
tw_lptype b_lp = {
  (init_f)init,
  (pre_run_f)NULL,
  (event_f)NULL,
  (revent_f)NULL,
  (commit_f)NULL,
  (final_f)NULL,
  (map_f)codes::CodesMapping,
  sizeof(state),
};
tw_lptype c_lp = {
  (init_f)init,
  (pre_run_f)NULL,
  (event_f)NULL,
  (revent_f)NULL,
  (commit_f)NULL,
  (final_f)NULL,
  (map_f)codes::CodesMapping,
  sizeof(state),
};

void CheckDestinationLPs(const std::string& sender_name, const std::string& dest_lp_name)
{
  //
}

static char conf_file_name[256] = { '\0' };
static const tw_optdef app_opt[] = { TWOPT_GROUP("codes-mapping test case"),
  TWOPT_CHAR("codes-config", conf_file_name, "name of codes configuration file"), TWOPT_END() };

int main(int argc, char* argv[])
{
  tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  if (!conf_file_name[0])
  {
    fprintf(stderr, "Expected \"codes-config\" option, please see --help.\n");
    MPI_Finalize();
    return 1;
  }

  auto& orchestrator = codes::Orchestrator::GetInstance();

  orchestrator.ParseConfig(conf_file_name);

  lp_type_register("a", &a_lp);
  lp_type_register("b", &b_lp);
  lp_type_register("c", &c_lp);

  auto mapper = orchestrator.GetMapper();
  mapper->MappingSetup();

  if (mapper->GetLPTypeCount("a") != 7)
  {
    std::cout << "Incorrect number of LPs of type 'a'. Expected 7, got "
              << mapper->GetLPTypeCount("a");
    return EXIT_FAILURE;
  }
  if (mapper->GetLPTypeCount("b") != 7)
  {
    std::cout << "Incorrect number of LPs of type 'b'. Expected 7, got "
              << mapper->GetLPTypeCount("c");
    return EXIT_FAILURE;
  }
  if (mapper->GetLPTypeCount("c") != 12)
  {
    std::cout << "Incorrect number of LPs of type 'c'. Expected 12, got "
              << mapper->GetLPTypeCount("c");
    return EXIT_FAILURE;
  }

  std::vector<std::string> lp_types = { "a", "b", "c" };
  std::vector<std::string> lp_names = { "a0", "a1", "a2", "a3", "a4", "a5", "a6", "b0", "b1", "b2",
    "b3", "b4", "b5", "b6", "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "c10",
    "c11" };

  // check lp type info
  for (int i = 0; i < g_tw_total_lps; i++)
  {
    std::string lp_type_name;
    int offset;
    mapper->GetLPTypeInfo(i, lp_type_name, offset);
    std::cout << "gid: " << i << " type: " << lp_type_name << " offset: " << offset << std::endl;
    auto gid = mapper->GetLPId(lp_type_name, offset);
    if (gid != i)
    {
      std::cout << "could not retrive correct lp id. expected " << i << ", got " << gid
                << std::endl;
    }

    auto relative_id = mapper->GetRelativeLPId(i);
    if (relative_id != offset)
    {
      std::cout << "did not get correct relative id for lp " << i << ". expected " << offset
                << ", got " << relative_id << std::endl;
    }
  }

  tw_run();
  tw_end();

  return EXIT_SUCCESS;
}
