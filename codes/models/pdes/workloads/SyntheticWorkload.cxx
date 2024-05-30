//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/models/pdes/workloads/SyntheticWorkload.h"

#include "codes/lp-type-lookup.h"
#include "codes/mapping/Mapper.h"
#include "codes/model-net/model-net.h"
#include "codes/orchestrator/Orchestrator.h"
#include "codes/util/CodesUtils.h"

namespace
{

const std::string LP_NAME = "SyntheticWorkload";
static int NETWORK_ID = 0;

static int traffic = 1;
static double arrival_time = 1000.0;
static int payload_size = 2048;
static int num_msgs = 20;
static unsigned long long num_nodes = 0;

} // end anon namespace

/* type of synthetic traffic */
// Only supporting Uniform for now
enum TRAFFIC
{
  UNIFORM = 1, /* sends message to a randomly selected node */
};

static void svr_init(SyntheticWorkloadState* ns, tw_lp* lp);
static void svr_event(SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp);
static void svr_rev_event(SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp);
static void svr_finalize(SyntheticWorkloadState* ns, tw_lp* lp);

tw_lptype SyntheticWorkloadLP = {
  (init_f)svr_init,
  (pre_run_f)NULL,
  (event_f)svr_event,
  (revent_f)svr_rev_event,
  (commit_f)NULL,
  (final_f)svr_finalize,
  (map_f)codes::CodesMapping,
  sizeof(SyntheticWorkloadState),
};

void SyntheticWorkloadRegisterLPType()
{
  lp_type_register(LP_NAME.c_str(), &SyntheticWorkloadLP);
}

void SyntheticWorkloadSetNetId(int net_id)
{
  NETWORK_ID = net_id;
}

const bool registered =
  codes::Orchestrator::GetInstance().RegisterLPType(codes::CodesLPTypes::SyntheticWorkload,
    SyntheticWorkloadRegisterLPType, SyntheticWorkloadSetNetId);

/* setup for the ROSS event tracing
 */
void svr_event_collect(SyntheticWorkloadMsg* m, tw_lp* lp, char* buffer, int* collect_flag)
{
  (void)lp;
  (void)collect_flag;
  int type = (int)m->svr_event_type;
  memcpy(buffer, &type, sizeof(type));
}

/* can add in any model level data to be collected along with simulation engine data
 * in the ROSS instrumentation.  Will need to update the last field in
 * svr_model_types[0] for the size of the data to save in each function call
 */
void svr_model_stat_collect(SyntheticWorkloadState* s, tw_lp* lp, char* buffer)
{
  (void)s;
  (void)lp;
  (void)buffer;
  return;
}

st_model_types svr_model_types[] = { { (ev_trace_f)svr_event_collect, sizeof(int),
                                       (model_stat_f)svr_model_stat_collect, 0, NULL, NULL, 0 },
  { NULL, 0, NULL, 0, NULL, NULL, 0 } };

static const st_model_types* svr_get_model_stat_types(void)
{
  return (&svr_model_types[0]);
}

void svr_register_model_types()
{
  st_model_type_register("nw-lp", svr_get_model_stat_types());
}

const tw_lptype* svr_get_lp_type()
{
  return (&SyntheticWorkloadLP);
}

static void svr_add_lp_type()
{
  lp_type_register("nw-lp", svr_get_lp_type());
}

static void issue_event(SyntheticWorkloadState* ns, tw_lp* lp)
{
  (void)ns;

  tw_event* e;
  SyntheticWorkloadMsg* m;
  tw_stime kickoff_time;

  /* each server sends a dummy event to itself that will kick off the real
   * simulation
   */

  /* skew each kickoff event slightly to help avoid event ties later on */
  kickoff_time = 1.1 * g_tw_lookahead + tw_rand_exponential(lp->rng, arrival_time);

  e = tw_event_new(lp->gid, kickoff_time, lp);
  m = reinterpret_cast<SyntheticWorkloadMsg*>(tw_event_data(e));
  m->svr_event_type = SyntheticWorkloadEventTypes::KICKOFF;
  tw_event_send(e);
}

static void svr_init(SyntheticWorkloadState* ns, tw_lp* lp)
{
  ns->start_ts = 0.0;
  auto& orchestrator = codes::Orchestrator::GetInstance();
  auto& lpConfig = orchestrator.GetLPConfig(LP_NAME);
  if (lpConfig.Properties.Has("traffic"))
  {
    auto traffic_type = lpConfig.Properties.GetString("traffic");
    if (traffic_type == "uniform")
    {
      traffic = UNIFORM;
    }
  }

  if (lpConfig.Properties.Has("arrival_time"))
  {
    arrival_time = lpConfig.Properties.GetDouble("arrival_time");
  }

  if (lpConfig.Properties.Has("num_messages"))
  {
    num_msgs = lpConfig.Properties.GetInt("num_messages");
  }

  if (lpConfig.Properties.Has("payload_size"))
  {
    payload_size = lpConfig.Properties.GetInt("payload_size");
  }

  auto& mapper = orchestrator.GetMapper();
  num_nodes = mapper.GetLPTypeCount(LP_NAME);

  issue_event(ns, lp);
  return;
}

static void handle_kickoff_rev_event(
  SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  if (m->incremented_flag)
    return;

  if (b->c1)
    tw_rand_reverse_unif(lp->rng);

  model_net_event_rc2(lp, &m->event_rc);
  ns->msg_sent_count--;
  tw_rand_reverse_unif(lp->rng);
}
static void handle_kickoff_event(
  SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  if (ns->msg_sent_count >= num_msgs)
  {
    m->incremented_flag = 1;
    return;
  }

  m->incremented_flag = 0;

  char anno[MAX_NAME_LENGTH];
  tw_lpid local_dest = -1, global_dest = -1;

  SyntheticWorkloadMsg* m_local =
    reinterpret_cast<SyntheticWorkloadMsg*>(malloc(sizeof(SyntheticWorkloadMsg)));
  SyntheticWorkloadMsg* m_remote =
    reinterpret_cast<SyntheticWorkloadMsg*>(malloc(sizeof(SyntheticWorkloadMsg)));

  m_local->svr_event_type = SyntheticWorkloadEventTypes::LOCAL;
  m_local->src = lp->gid;

  memcpy(m_remote, m_local, sizeof(SyntheticWorkloadMsg));
  m_remote->svr_event_type = SyntheticWorkloadEventTypes::REMOTE;

  // assert(NETWORK_ID == DRAGONFLY); /* only supported for dragonfly model right now. */
  ns->start_ts = tw_now(lp);
  auto& mapper = codes::Orchestrator::GetInstance().GetMapper();
  int local_id = mapper.GetRelativeLPId(lp->gid);
  std::string lp_type_name = mapper.GetLPTypeName(lp->gid);

  /* in case of uniform random traffic, send to a random destination. */
  if (traffic == UNIFORM)
  {
    b->c1 = 1;
    local_dest = tw_rand_integer(lp->rng, 0, num_nodes - 1);
  }
  assert(local_dest < num_nodes);
  global_dest = mapper.GetLPIdFromRelativeId(local_dest, lp_type_name);
  ns->msg_sent_count++;
  m->event_rc = model_net_event(NETWORK_ID, "test", global_dest, payload_size, 0.0,
    sizeof(SyntheticWorkloadMsg), (const void*)m_remote, sizeof(SyntheticWorkloadMsg),
    (const void*)m_local, lp);

  issue_event(ns, lp);
  return;
}

static void handle_remote_rev_event(
  SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  (void)b;
  (void)m;
  (void)lp;
  ns->msg_recvd_count--;
}

static void handle_remote_event(
  SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  (void)b;
  (void)m;
  (void)lp;
  ns->msg_recvd_count++;
}

static void handle_local_rev_event(
  SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  (void)b;
  (void)m;
  (void)lp;
  ns->local_recvd_count--;
}

static void handle_local_event(
  SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  (void)b;
  (void)m;
  (void)lp;
  ns->local_recvd_count++;
}

static void svr_finalize(SyntheticWorkloadState* ns, tw_lp* lp)
{
  ns->end_ts = tw_now(lp);

  printf("server %llu recvd %d bytes in %f seconds, %f MiB/s sent_count %d recvd_count %d "
         "local_count %d \n",
    (unsigned long long)lp->gid, payload_size * ns->msg_recvd_count,
    codes::NSToSeconds(ns->end_ts - ns->start_ts),
    ((double)(payload_size * ns->msg_sent_count) / (double)(1024 * 1024) /
      codes::NSToSeconds(ns->end_ts - ns->start_ts)),
    ns->msg_sent_count, ns->msg_recvd_count, ns->local_recvd_count);
  return;
}

static void svr_rev_event(SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  switch (m->svr_event_type)
  {
    case SyntheticWorkloadEventTypes::REMOTE:
      handle_remote_rev_event(ns, b, m, lp);
      break;
    case SyntheticWorkloadEventTypes::LOCAL:
      handle_local_rev_event(ns, b, m, lp);
      break;
    case SyntheticWorkloadEventTypes::KICKOFF:
      handle_kickoff_rev_event(ns, b, m, lp);
      break;
    default:
      assert(0);
      break;
  }
}

static void svr_event(SyntheticWorkloadState* ns, tw_bf* b, SyntheticWorkloadMsg* m, tw_lp* lp)
{
  switch (m->svr_event_type)
  {
    case SyntheticWorkloadEventTypes::REMOTE:
      handle_remote_event(ns, b, m, lp);
      break;
    case SyntheticWorkloadEventTypes::LOCAL:
      handle_local_event(ns, b, m, lp);
      break;
    case SyntheticWorkloadEventTypes::KICKOFF:
      handle_kickoff_event(ns, b, m, lp);
      break;
    default:
      printf("\n Invalid message type %d ", static_cast<int>(m->svr_event_type));
      assert(0);
      break;
  }
}
