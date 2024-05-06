/*
 * Copyright (C) 2014 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 */

#include <assert.h>
#include <cstring>
#include <stddef.h>

#include "codes/GlobalDefines.h"
#include "codes/codes.h"
#include "codes/lp-type-lookup.h"
#include "codes/mapping/Mapper.h"
#include "codes/model-net/model-net-lp.h"
#include "codes/model-net/model-net-method.h"
#include "codes/model-net/model-net-sched.h"
#include "codes/model-net/model-net.h"
#include "codes/orchestrator/Orchestrator.h"
#include "codes/util/jenkins-hash.h"

#define MN_NAME "model_net_base"

#define DEBUG 1
/**** BEGIN SIMULATION DATA STRUCTURES ****/

int model_net_base_magic;
int mn_sample_enabled = 0;

// message-type specific offsets - don't want to get bitten later by alignment
// issues...
static int msg_offsets[MAX_NETS];

void SetMsgOffsets(int i, int offset)
{
  msg_offsets[i] = offset;
}

/* annotation-specific parameters (unannotated entry occurs at the
 * last index) */
static int num_params = 0;
static const char* annos[CONFIGURATION_MAX_ANNOS];

void SetAnnos(int i, const char* anno)
{
  annos[i] = anno;
}

static model_net_base_params all_params[CONFIGURATION_MAX_ANNOS];

model_net_base_params* GetAllParams(int i)
{
  return &all_params[i];
}

static tw_stime mn_sample_interval = 0.0;
static tw_stime mn_sample_end = 0.0;
static int servers_per_node_queue = -1;
extern tw_stime codes_cn_delay;

tw_lptype model_net_base_lp = {
  (init_f)model_net_base_lp_init,
  (pre_run_f)NULL,
  (event_f)model_net_base_event,
  (revent_f)model_net_base_event_rc,
  (commit_f)model_net_commit_event,
  (final_f)model_net_base_finalize,
  (map_f)codes::CodesMapping,
  sizeof(model_net_base_state),
};

/**** END SIMULATION DATA STRUCTURES ****/

/**** BEGIN LP, EVENT PROCESSING FUNCTION DECLS ****/

void model_net_commit_event(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp)
{
  if (m->h.event_type == MN_BASE_PASS)
  {
    void* sub_msg;
    sub_msg = ((char*)m) + msg_offsets[ns->net_id];

    if (ns->sub_type->commit != NULL)
      ns->sub_type->commit(ns->sub_state, b, sub_msg, lp);
  }

  if (m->h.event_type == MN_CONGESTION_EVENT)
  {
    void* sub_msg;
    sub_msg = ((char*)m) + msg_offsets[CONGESTION_CONTROLLER];
    commit_f con_ev_commit = method_array[ns->net_id]->cc_congestion_event_commit_fn;
    if (con_ev_commit != NULL)
      con_ev_commit(ns->sub_state, b, sub_msg, lp);
  }
}
/* setup for the ROSS event tracing
 */
void mn_event_collect(model_net_wrap_msg* m, tw_lp* lp, char* buffer, int* collect_flag)
{
  // assigning large numbers to message types to make it easier to
  // determine which messages are model net base LP msgs
  int type;
  void* sub_msg;
  switch (m->h.event_type)
  {
    case MN_BASE_NEW_MSG:
      type = 9000;
      memcpy(buffer, &type, sizeof(type));
      break;
    case MN_BASE_SCHED_NEXT:
      type = 9001;
      memcpy(buffer, &type, sizeof(type));
      break;
    case MN_BASE_SAMPLE:
      type = 9002;
      memcpy(buffer, &type, sizeof(type));
      break;
    case MN_BASE_PASS:
      sub_msg = ((char*)m) + msg_offsets[((model_net_base_state*)lp->cur_state)->net_id];
      if (((model_net_base_state*)lp->cur_state)->sub_model_type)
      {
        if (g_st_ev_trace)
          (((model_net_base_state*)lp->cur_state)->sub_model_type->ev_trace)(
            sub_msg, lp, buffer, collect_flag);
      }
      break;
    default: // this shouldn't happen, but can help detect an issue
      type = 9004;
      break;
  }
}

void mn_model_stat_collect(model_net_base_state* s, tw_lp* lp, char* buffer)
{
  // need to call the model level stats collection fn
  if (s->sub_model_type)
    (*s->sub_model_type->model_stat_fn)(s->sub_state, lp, buffer);
  return;
}

void mn_sample_event(model_net_base_state* s, tw_bf* bf, tw_lp* lp, void* sample)
{
  if (s->sub_model_type)
    (*s->sub_model_type->sample_event_fn)(s->sub_state, bf, lp, sample);
}

void mn_sample_rc_event(model_net_base_state* s, tw_bf* bf, tw_lp* lp, void* sample)
{
  if (s->sub_model_type)
    (*s->sub_model_type->sample_revent_fn)(s->sub_state, bf, lp, sample);
}

st_model_types mn_model_types[MAX_NETS];

st_model_types mn_model_base_type = { (ev_trace_f)mn_event_collect, sizeof(int),
  (model_stat_f)mn_model_stat_collect, 0, (sample_event_f)mn_sample_event,
  (sample_revent_f)mn_sample_rc_event, 0 };

/**** END LP, EVENT PROCESSING FUNCTION DECLS ****/

/**** BEGIN IMPLEMENTATIONS ****/

void model_net_enable_sampling(tw_stime interval, tw_stime end)
{
  mn_sample_interval = interval;
  mn_sample_end = end;
  mn_sample_enabled = 1;
}

int model_net_sampling_enabled(void)
{
  return mn_sample_enabled;
}

// schedule sample event - want to be precise, so no noise here
static void issue_sample_event(tw_lp* lp)
{
  if (tw_now(lp) + mn_sample_interval < mn_sample_end + 0.0001)
  {
    tw_event* e = tw_event_new(lp->gid, mn_sample_interval, lp);
    model_net_wrap_msg* m = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
    msg_set_header(model_net_base_magic, MN_BASE_SAMPLE, lp->gid, &m->h);
    tw_event_send(e);
  }
}

void model_net_base_register(int* do_config_nets)
{
  // here, we initialize ALL lp types to use the base type
  for (int i = 0; i < MAX_NETS; i++)
  {
    if (do_config_nets[i])
    {
      // some model-net lps need custom registration hooks (dragonfly).
      // Those that don't NULL out the reg. function
      if (method_array[i]->mn_register == NULL)
        lp_type_register(model_net_lp_config_names[i], &model_net_base_lp);
      else
        method_array[i]->mn_register(&model_net_base_lp);
      if (g_st_ev_trace || g_st_model_stats || g_st_use_analysis_lps) // for ROSS event tracing
      {
        if (method_array[i]->mn_model_stat_register != NULL)
        //    st_model_type_register(model_net_lp_config_names[i], &mn_model_types[i]);
        // else
        {
          memcpy(&mn_model_types[i], &mn_model_base_type, sizeof(st_model_types));
          method_array[i]->mn_model_stat_register(&mn_model_types[i]);
        }
      }
    }
  }
}

static void base_read_config(const char* anno, model_net_base_params* p)
{
  long int packet_size_l = 0;
  uint64_t packet_size;

  auto& orchestrator = codes::Orchestrator::GetInstance();
  auto parser = orchestrator.GetConfigParser();
  auto& simConfig = parser->GetSimulationConfig();
  packet_size = simConfig.PacketSize;

  if (!simConfig.ModelNetScheduler.empty())
  {
    int i;
    for (i = 0; i < MAX_SCHEDS; i++)
    {
      if (sched_names[i] == simConfig.ModelNetScheduler)
      {
        p->sched_params.type = static_cast<sched_type>(i);
        break;
      }
    }
    if (i == MAX_SCHEDS)
    {
      tw_error(TW_LOC,
        "Unknown value for PARAMS:modelnet-scheduler : "
        "%s",
        simConfig.ModelNetScheduler.c_str());
    }
  }
  else
  {
    // default: FCFS
    p->sched_params.type = MN_SCHED_FCFS;
  }

  // TODO: need to support the following config options
  p->num_queues = 1;
  if (!g_tw_mynode)
  {
    fprintf(stdout,
      "NIC num injection port not specified, "
      "setting to %d\n",
      p->num_queues);
  }

  p->nic_seq_delay = 10;
  if (!g_tw_mynode)
  {
    fprintf(stdout,
      "NIC seq delay not specified, "
      "setting to %lf\n",
      p->nic_seq_delay);
  }

  p->node_copy_queues = 1;
  if (!g_tw_mynode)
  {
    fprintf(stdout,
      "NIC num copy queues not specified, "
      "setting to %d\n",
      p->node_copy_queues);
  }

  // get scheduler-specific parameters
  // TODO:
  // if (p->sched_params.type == MN_SCHED_PRIO)
  // {
  //   // prio scheduler uses default parameters
  //   int* num_prios = &p->sched_params.u.prio.num_prios;
  //   enum sched_type* sub_stype = &p->sched_params.u.prio.sub_stype;
  //   // number of priorities to allocate
  //   ret = configuration_get_value_int(&config, "PARAMS", "prio-sched-num-prios", anno,
  //   num_prios); if (ret != 0)
  //     *num_prios = 10;
  //
  //   ret = configuration_get_value(
  //     &config, "PARAMS", "prio-sched-sub-sched", anno, sched, MAX_NAME_LENGTH);
  //   if (ret <= 0)
  //     *sub_stype = MN_SCHED_FCFS;
  //   else
  //   {
  //     int i;
  //     for (i = 0; i < MAX_SCHEDS; i++)
  //     {
  //       if (strcmp(sched_names[i], sched) == 0)
  //       {
  //         *sub_stype = static_cast<sched_type>(i);
  //         break;
  //       }
  //     }
  //     if (i == MAX_SCHEDS)
  //     {
  //       tw_error(TW_LOC,
  //         "Unknown value for "
  //         "PARAMS:prio-sched-sub-sched %s",
  //         sched);
  //     }
  //     else if (i == MN_SCHED_PRIO)
  //     {
  //       tw_error(TW_LOC, "priority scheduler cannot be used as a "
  //                        "priority scheduler's sub sched "
  //                        "(PARAMS:prio-sched-sub-sched)");
  //     }
  //   }
  // }

  if (p->sched_params.type == MN_SCHED_FCFS_FULL ||
      (p->sched_params.type == MN_SCHED_PRIO &&
        p->sched_params.u.prio.sub_stype == MN_SCHED_FCFS_FULL))
  {
    // override packet size to something huge (leave a bit in the unlikely
    // case that an op using packet size causes overflow)
    packet_size = 1ull << 62;
  }
  else if (!packet_size && (p->sched_params.type != MN_SCHED_FCFS_FULL ||
                             (p->sched_params.type == MN_SCHED_PRIO &&
                               p->sched_params.u.prio.sub_stype != MN_SCHED_FCFS_FULL)))
  {
    packet_size = 512;
    fprintf(stderr,
      "WARNING, no packet size specified, setting packet "
      "size to %llu\n",
      LLU(packet_size));
  }

  p->packet_size = packet_size;
}

void model_net_base_configure()
{
  uint32_t h1 = 0, h2 = 0;

  bj_hashlittle2(MN_NAME, strlen(MN_NAME), &h1, &h2);
  model_net_base_magic = h1 + h2;

  // set up offsets - doesn't matter if they are actually used or not
  msg_offsets[SIMPLENET] = offsetof(model_net_wrap_msg, msg.m_snet);
  msg_offsets[SIMPLEP2P] = offsetof(model_net_wrap_msg, msg.m_sp2p);
  msg_offsets[CONGESTION_CONTROLLER] = offsetof(model_net_wrap_msg, msg.m_cc);

  // perform the configuration(s)
  // This part is tricky, as we basically have to look up all annotations that
  // have LP names of the form modelnet_*. For each of those, we need to read
  // the base parameters
  // - the init is a little easier as we can use the LP-id to look up the
  // annotation
  auto& orchestrator = codes::Orchestrator::GetInstance();
  auto parser = orchestrator.GetConfigParser();
  auto& simConfig = parser->GetSimulationConfig();
  auto& lpConfigs = parser->GetLPTypeConfigs();

  // first grab all of the annotations and store locally
  // TODO: I'm not really sure about what the annotation stuff is
  // but I think we need an entry in annos and all_params for each model net lp
  for (int c = 0; c < lpConfigs.size(); c++)
  {
    const auto& lpConf = lpConfigs[c];
    if (lpConf.ModelName.find("modelnet_") != std::string::npos)
    {
      // TODO: handle annotations
      // we're just assuming they're all unannotated for now
      // if (amap->has_unanno_lp)
      {
        int a;
        for (a = 0; a < num_params; a++)
        {
          if (annos[a] == NULL)
            break;
        }
        if (a == num_params)
        {
          // found a new (empty) annotation
          annos[num_params++] = NULL;
        }
      }
    }
  }

  // now that we have all of the annos for all of the networks, loop through
  // and read the configs
  for (int i = 0; i < num_params; i++)
  {
    base_read_config(annos[i], &all_params[i]);
  }
}

void model_net_base_lp_init(model_net_base_state* ns, tw_lp* lp)
{
  // obtain the underlying lp type through codes-mapping
  char lp_type_name[MAX_NAME_LENGTH], anno[MAX_NAME_LENGTH], group[MAX_NAME_LENGTH];
  int dummy;

  auto mapper = codes::Orchestrator::GetInstance().GetMapper();
  ns->params = &all_params[0];
  auto typeName = mapper->GetLPTypeName(lp->gid);
  strncpy(lp_type_name, typeName.c_str(), typeName.size());

  // find the corresponding method name / index
  for (int i = 0; i < MAX_NETS; i++)
  {
    if (strcmp(model_net_lp_config_names[i], lp_type_name) == 0)
    {
      ns->net_id = i;
      break;
    }
  }

  // so for simplep2p example this should actually be 1, not 3, so it's not just simply getting the
  // count of a specifc LP type
  // so need to check with dragonfly sim then, so i think for the terminal lp, it would be that
  // count within a repetition and similarly for the router lp
  // so then in our case, it should just be 1 for now?
  // TODO:
  ns->nics_per_router = 1; // mapper->GetLPCount(lp_type_name);

  ns->msg_id = 0;
  ns->next_available_time = 0;
  ns->node_copy_next_available_time =
    (tw_stime*)malloc(ns->params->node_copy_queues * sizeof(tw_stime));
  for (int i = 0; i < ns->params->node_copy_queues; i++)
  {
    ns->node_copy_next_available_time[i] = 0;
  }

  ns->in_sched_send_loop = (int*)malloc(ns->params->num_queues * sizeof(int));
  ns->sched_send = (model_net_sched**)malloc(ns->params->num_queues * sizeof(model_net_sched*));
  for (int i = 0; i < ns->params->num_queues; i++)
  {
    ns->sched_send[i] = (model_net_sched*)malloc(sizeof(model_net_sched));
    model_net_sched_init(&ns->params->sched_params, 0, method_array[ns->net_id], ns->sched_send[i]);
    ns->in_sched_send_loop[i] = 0;
  }
  ns->sched_recv = reinterpret_cast<model_net_sched_s*>(malloc(sizeof(model_net_sched)));
  model_net_sched_init(&ns->params->sched_params, 1, method_array[ns->net_id], ns->sched_recv);

  ns->sub_type = model_net_get_lp_type(ns->net_id);

  /* some ROSS instrumentation setup */
  if (g_st_ev_trace || g_st_model_stats || g_st_use_analysis_lps)
  {
    ns->sub_model_type = model_net_get_model_stat_type(ns->net_id);
    if (ns->sub_model_type)
    {
      mn_model_types[ns->net_id].mstat_sz = ns->sub_model_type->mstat_sz;
      mn_model_types[ns->net_id].sample_struct_sz = ns->sub_model_type->sample_struct_sz;
    }
  }

  // NOTE: some models actually expect LP state to be 0 initialized...
  // *cough anything that uses mn_stats_array cough*
  ns->sub_state = calloc(1, ns->sub_type->state_sz);

  // initialize the model-net method
  ns->sub_type->init(ns->sub_state, lp);

  // check validity of sampling function
  event_f sample = method_array[ns->net_id]->mn_sample_fn;
  revent_f rsample = method_array[ns->net_id]->mn_sample_rc_fn;
  if (model_net_sampling_enabled())
  {
    if (sample == NULL)
    {
      /* MM: Commented out temporarily--- */
      // tw_error(TW_LOC,
      //         "Sampling requested for a model that doesn't provide it\n");
    }
    else if (rsample == NULL && (g_tw_synchronization_protocol == OPTIMISTIC ||
                                  g_tw_synchronization_protocol == OPTIMISTIC_DEBUG))
    {
      /* MM: Commented out temporarily--- */
      // tw_error(TW_LOC,
      //         "Sampling requested for a model that doesn't provide it\n");
    }
    else
    {
      init_f sinit = method_array[ns->net_id]->mn_sample_init_fn;
      if (sinit != NULL)
        sinit(ns->sub_state, lp);
      issue_sample_event(lp);
    }
  }
}

void model_net_base_event(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp)
{

  if (m->h.magic != model_net_base_magic)
    printf("\n LP ID mismatched %llu\n", LLU(lp->gid));

  assert(m->h.magic == model_net_base_magic);

  void* sub_msg;
  switch (m->h.event_type)
  {
    case MN_BASE_NEW_MSG:
      handle_new_msg(ns, b, m, lp);
      break;
    case MN_BASE_SCHED_NEXT:
      handle_sched_next(ns, b, m, lp);
      break;
    case MN_BASE_SAMPLE:
    {
      event_f sample = method_array[ns->net_id]->mn_sample_fn;
      assert(model_net_sampling_enabled() && sample != NULL);
      sub_msg = ((char*)m) + msg_offsets[ns->net_id];
      sample(ns->sub_state, b, sub_msg, lp);
      issue_sample_event(lp);
      break;
    }
    case MN_BASE_PASS:
    {
      sub_msg = ((char*)m) + msg_offsets[ns->net_id];
      ns->sub_type->event(ns->sub_state, b, sub_msg, lp);
      break;
    }
    case MN_BASE_END_NOTIF:
    {
      event_f end_ev = method_array[ns->net_id]->mn_end_notif_fn;
      end_ev(ns->sub_state, b, m, lp);
      break;
    }
    case MN_CONGESTION_EVENT:
    {
      event_f con_ev = method_array[ns->net_id]->cc_congestion_event_fn;
      assert(g_congestion_control_enabled && con_ev != NULL);
      sub_msg = ((char*)m) + msg_offsets[CONGESTION_CONTROLLER];
      con_ev(ns->sub_state, b, sub_msg, lp);
      break;
    }
    /* ... */
    default:
      assert(!"model_net_base event type not known");
      break;
  }
}

void model_net_base_event_rc(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp)
{
  assert(m->h.magic == model_net_base_magic);

  void* sub_msg;
  switch (m->h.event_type)
  {
    case MN_BASE_NEW_MSG:
      handle_new_msg_rc(ns, b, m, lp);
      break;
    case MN_BASE_SCHED_NEXT:
      handle_sched_next_rc(ns, b, m, lp);
      break;
    case MN_BASE_SAMPLE:;
      {
        revent_f sample_rc = method_array[ns->net_id]->mn_sample_rc_fn;
        assert(model_net_sampling_enabled() && sample_rc != NULL);
        sub_msg = ((char*)m) + msg_offsets[ns->net_id];
        sample_rc(ns->sub_state, b, sub_msg, lp);
        break;
      }
    case MN_BASE_PASS:;
      {
        sub_msg = ((char*)m) + msg_offsets[ns->net_id];
        ns->sub_type->revent(ns->sub_state, b, sub_msg, lp);
        break;
      }
    case MN_BASE_END_NOTIF:;
      {
        event_f end_ev_rc = method_array[ns->net_id]->mn_end_notif_rc_fn;
        end_ev_rc(ns->sub_state, b, m, lp);
        break;
      }
    case MN_CONGESTION_EVENT:;
      {
        revent_f con_ev_rc = method_array[ns->net_id]->cc_congestion_event_rc_fn;
        assert(g_congestion_control_enabled && con_ev_rc != NULL);
        sub_msg = ((char*)m) + msg_offsets[CONGESTION_CONTROLLER];
        con_ev_rc(ns->sub_state, b, sub_msg, lp);
        break;
      }
    /* ... */
    default:
      assert(!"model_net_base event type not known");
      break;
  }
}

void model_net_base_finalize(model_net_base_state* ns, tw_lp* lp)
{
  final_f sfini = method_array[ns->net_id]->mn_sample_fini_fn;
  if (sfini != NULL)
    sfini(ns->sub_state, lp);
  ns->sub_type->final(ns->sub_state, lp);
  free(ns->sub_state);
}

/// bitfields used:
/// c31 - we initiated a sched_next event
void handle_new_msg(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp)
{
#if DEBUG
  printf("%llu Entered handle_new_msg()\n", LLU(tw_now(lp)));
#endif
  auto mapper = codes::Orchestrator::GetInstance().GetMapper();
  static int num_servers = -1;
  static int servers_per_node = -1;
  if (num_servers == -1)
  {
    model_net_request* r = &m->msg.m_base.req;
    std::string lp_name = mapper->GetLPTypeName(lp->gid);
    num_servers = mapper->GetDestinationLPCount(r->src_lp, lp_name);
    servers_per_node = num_servers / ns->params->num_queues; // this is for entire switch
    if (servers_per_node == 0)
      servers_per_node = 1;
    servers_per_node_queue = num_servers / ns->nics_per_router / ns->params->node_copy_queues;
    if (servers_per_node_queue == 0)
      servers_per_node_queue = 1;
    if (!g_tw_mynode)
    {
      fprintf(stdout,
        "Set num_servers per router %d, servers per "
        "injection queue per router %d, servers per node copy queue "
        "per node %d, num nics %d\n",
        num_servers, servers_per_node, servers_per_node_queue, ns->nics_per_router);
    }
  }

  if (lp->gid == m->msg.m_base.req.dest_mn_lp)
  {
    model_net_request* r = &m->msg.m_base.req;
    // FIXME: need to figure out what offset should be here
    int offset = 0;
    int queue = offset / ns->nics_per_router / servers_per_node_queue;
    m->msg.m_base.save_ts = ns->node_copy_next_available_time[queue];
    tw_stime exp_time = ((ns->node_copy_next_available_time[queue] > tw_now(lp))
                           ? ns->node_copy_next_available_time[queue]
                           : tw_now(lp));
    exp_time += r->msg_size * codes_cn_delay;
    exp_time -= tw_now(lp);
    tw_stime delay = codes_local_latency(lp);
    ns->node_copy_next_available_time[queue] = tw_now(lp) + exp_time;
    // ns->node_copy_next_available_time[queue] = exp_time;
    int remote_event_size = r->remote_event_size;
    int self_event_size = r->self_event_size;
    void* e_msg = (m + 1);
    if (remote_event_size > 0)
    {
      exp_time += delay;
      tw_event* e = tw_event_new(r->final_dest_lp, exp_time, lp);
      memcpy(tw_event_data(e), e_msg, remote_event_size);
      tw_event_send(e);
      e_msg = (char*)e_msg + remote_event_size;
    }
    if (self_event_size > 0)
    {
      exp_time += delay;
      tw_event* e = tw_event_new(r->src_lp, exp_time, lp);
      memcpy(tw_event_data(e), e_msg, self_event_size);
      tw_event_send(e);
    }
    return;
  }

  if (m->msg.m_base.isQueueReq)
  {
    m->msg.m_base.save_ts = ns->next_available_time;
    tw_stime exp_time =
      ((ns->next_available_time > tw_now(lp)) ? ns->next_available_time : tw_now(lp));
    exp_time += ns->params->nic_seq_delay + codes_local_latency(lp);
    ns->next_available_time = exp_time;
    tw_event* e = tw_event_new(lp->gid, exp_time - tw_now(lp), lp);
    model_net_wrap_msg* m_new = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
    memcpy(m_new, m, sizeof(model_net_wrap_msg));
    void* e_msg = (m + 1);
    void* e_new_msg = (m_new + 1);
    model_net_request* r = &m->msg.m_base.req;
    int remote_event_size = r->remote_event_size;
    int self_event_size = r->self_event_size;
    if (remote_event_size > 0)
    {
      memcpy(e_new_msg, e_msg, remote_event_size);
      e_msg = (char*)e_msg + remote_event_size;
      e_new_msg = (char*)e_new_msg + remote_event_size;
    }
    if (self_event_size > 0)
    {
      memcpy(e_new_msg, e_msg, self_event_size);
    }
    m_new->msg.m_base.isQueueReq = 0;
    tw_event_send(e);
#if DEBUG
    printf("%llu isQueueReq and dropping outof handle_new_msg(\n", LLU(tw_now(lp)));
#endif
    return;
  }
  // simply pass down to the scheduler
  model_net_request* r = &m->msg.m_base.req;
  // don't forget to set packet size, now that we're responsible for it!
  r->packet_size = ns->params->packet_size;
  r->msg_id = ns->msg_id++;
  void* m_data = m + 1;
  void *remote = NULL, *local = NULL;
  if (r->remote_event_size > 0)
  {
    remote = m_data;
    m_data = (char*)m_data + r->remote_event_size;
  }
  if (r->self_event_size > 0)
  {
    local = m_data;
  }

  int queue_offset = 0;
  if (!m->msg.m_base.is_from_remote && ns->params->num_queues != 1)
  {
    // FIXME: need to figure out what offset should be here
    int offset = 0;
    std::string lp_name = mapper->GetLPTypeName(lp->gid);
    if (num_servers == -1)
    {
      num_servers = mapper->GetDestinationLPCount(r->src_lp, lp_name);
      servers_per_node = num_servers / ns->params->num_queues;
      if (servers_per_node == 0)
        servers_per_node = 1;
    }
#if DEBUG
    printf("r->src_lp:%llu, num_servers:%d num_queues:%d, offset:%d servers_per_node:%d\n",
      LLU(r->src_lp), num_servers, ns->params->num_queues, offset, servers_per_node);
#endif
    queue_offset = (offset / servers_per_node) % ns->params->num_queues;
  }
  r->queue_offset = queue_offset;
#if DEBUG
  printf("queue_offset:%d\n", queue_offset);
#endif
  // printf("num_queues:%d q0_loop:%d
  // q1_loop:%d\n",ns->params->num_queues,ns->in_sched_send_loop[0], ns->in_sched_send_loop[1]);

  // for(int j=0; j<ns->params->num_queues; j++){
  //     queue_offset = j;
  //     r->queue_offset = j;
  //  set message-specific params
  int is_from_remote = m->msg.m_base.is_from_remote;
  model_net_sched* ss = is_from_remote ? ns->sched_recv : ns->sched_send[queue_offset];
  int* in_sched_loop =
    is_from_remote ? &ns->in_sched_recv_loop : &ns->in_sched_send_loop[queue_offset];
  model_net_sched_add(r, &m->msg.m_base.sched_params, r->remote_event_size, remote,
    r->self_event_size, local, ss, &m->msg.m_base.rc, lp);

  if (*in_sched_loop == 0)
  {
    b->c31 = 1;
    /* No need to issue an extra sched-next event if we're currently idle */
    *in_sched_loop = 1;
    /* NOTE: we can do this because the sched rc struct in the event is
     * *very* lightly used (there's harmless overlap in usage for the
     * priority scheduler) */
#if DEBUG
    printf("%llu handle_shed_next() from handle_new_msg()\n", LLU(tw_now(lp)));
#endif
    handle_sched_next(ns, b, m, lp);
    assert(*in_sched_loop);
  }
}

void handle_new_msg_rc(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp)
{
  if (lp->gid == m->msg.m_base.req.dest_mn_lp)
  {
    codes_local_latency_reverse(lp);
    model_net_request* r = &m->msg.m_base.req;
    // FIXME: offset?
    int offset = 0;
    int queue = offset / ns->nics_per_router / servers_per_node_queue;
    ns->node_copy_next_available_time[queue] = m->msg.m_base.save_ts;
    return;
  }
  if (m->msg.m_base.isQueueReq)
  {
    codes_local_latency_reverse(lp);
    ns->next_available_time = m->msg.m_base.save_ts;
    return;
  }
  model_net_request* r = &m->msg.m_base.req;
  int is_from_remote = m->msg.m_base.is_from_remote;
  model_net_sched* ss = is_from_remote ? ns->sched_recv : ns->sched_send[r->queue_offset];
  int* in_sched_loop =
    is_from_remote ? &ns->in_sched_recv_loop : &ns->in_sched_send_loop[r->queue_offset];

  if (b->c31)
  {
    handle_sched_next_rc(ns, b, m, lp);
    *in_sched_loop = 0;
  }
  model_net_sched_add_rc(ss, &m->msg.m_base.rc, lp);
}

/// bitfields used
/// c0 - scheduler loop is finished
void handle_sched_next(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp)
{
#if DEBUG
  printf("%llu handle sched_next function\n", LLU(tw_now(lp)));
#endif
  tw_stime poffset;
  model_net_request* r = &m->msg.m_base.req;
  int is_from_remote = m->msg.m_base.is_from_remote;
  model_net_sched* ss = is_from_remote ? ns->sched_recv : ns->sched_send[r->queue_offset];
  int* in_sched_loop =
    is_from_remote ? &ns->in_sched_recv_loop : &ns->in_sched_send_loop[r->queue_offset];
  int ret = model_net_sched_next(&poffset, ss, m + 1, &m->msg.m_base.rc, lp);
  // we only need to know whether scheduling is finished or not - if not,
  // go to the 'next iteration' of the loop
#if DEBUG
  printf("return value from model_net_sched_next(): %d in_sched_loop changing from %d to 0\n", ret,
    *in_sched_loop);
#endif
  if (ret == -1)
  {
    b->c0 = 1;
    *in_sched_loop = 0;
  }
  // Currently, only a subset of the network implementations use the
  // callback-based scheduling loop (model_net_method_idle_event).
  // For all others, we need to schedule the next packet
  // immediately
  else if (ns->net_id == SIMPLEP2P)
  {
    tw_event* e = tw_event_new(lp->gid, poffset + codes_local_latency(lp), lp);
    model_net_wrap_msg* m_wrap = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
    model_net_request* r_wrap = &m_wrap->msg.m_base.req;
    msg_set_header(model_net_base_magic, MN_BASE_SCHED_NEXT, lp->gid, &m_wrap->h);
    m_wrap->msg.m_base.is_from_remote = is_from_remote;
    r_wrap->queue_offset = r->queue_offset;
    // no need to set m_base here
    tw_event_send(e);
  }
}

void handle_sched_next_rc(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp)
{
  model_net_request* r = &m->msg.m_base.req;
  int is_from_remote = m->msg.m_base.is_from_remote;
  model_net_sched* ss = is_from_remote ? ns->sched_recv : ns->sched_send[r->queue_offset];
  int* in_sched_loop =
    is_from_remote ? &ns->in_sched_recv_loop : &ns->in_sched_send_loop[r->queue_offset];

  model_net_sched_next_rc(ss, m + 1, &m->msg.m_base.rc, lp);
  if (b->c0)
  {
    *in_sched_loop = 1;
  }
  else if (ns->net_id == SIMPLEP2P)
  {
    codes_local_latency_reverse(lp);
  }
}

/**** END IMPLEMENTATIONS ****/

tw_event* model_net_method_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp* sender,
  int net_id, void** msg_data, void** extra_data)
{
  tw_event* e = tw_event_new(dest_gid, offset_ts, sender);
  model_net_wrap_msg* m_wrap = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
  msg_set_header(model_net_base_magic, MN_BASE_PASS, sender->gid, &m_wrap->h);
  *msg_data = ((char*)m_wrap) + msg_offsets[net_id];
  // extra_data is optional
  if (extra_data != NULL)
  {
    *extra_data = m_wrap + 1;
  }
  return e;
}

void model_net_method_send_msg_recv_event(tw_lpid final_dest_lp, tw_lpid dest_mn_lp,
  tw_lpid src_lp, // the "actual" source (as opposed to the model net lp)
  uint64_t msg_size, int is_pull, uint64_t pull_size, int remote_event_size,
  const mn_sched_params* sched_params, const char* category, int net_id, void* msg, tw_stime offset,
  tw_lp* sender)
{
  tw_event* e = tw_event_new(dest_mn_lp, offset + codes_local_latency(sender), sender);
  model_net_wrap_msg* m = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
  msg_set_header(model_net_base_magic, MN_BASE_NEW_MSG, sender->gid, &m->h);

  if (sched_params != NULL)
    m->msg.m_base.sched_params = *sched_params;
  else
    model_net_sched_set_default_params(&m->msg.m_base.sched_params);

  model_net_request* r = &m->msg.m_base.req;
  r->final_dest_lp = final_dest_lp;
  r->src_lp = src_lp;
  // for "recv" events, set the "dest" to this LP in the case of a pull event
  r->dest_mn_lp = sender->gid;
  r->pull_size = pull_size;
  r->msg_size = msg_size;
  // TODO: document why we're setting packet_size this way
  r->packet_size = msg_size;
  r->net_id = net_id;
  r->is_pull = is_pull;
  r->remote_event_size = remote_event_size;
  r->self_event_size = 0;
  m->msg.m_base.is_from_remote = 1;

  strncpy(r->category, category, CATEGORY_NAME_MAX - 1);
  r->category[CATEGORY_NAME_MAX - 1] = '\0';

  if (remote_event_size > 0)
  {
    void* m_dat = model_net_method_get_edata(net_id, msg);
    memcpy(m + 1, m_dat, remote_event_size);
  }

  tw_event_send(e);
}

void model_net_method_send_msg_recv_event_rc(tw_lp* sender)
{
  codes_local_latency_reverse(sender);
}

void model_net_method_idle_event(tw_stime offset_ts, int is_recv_queue, tw_lp* lp)
{
  model_net_method_idle_event2(offset_ts, is_recv_queue, 0, lp);
}

void model_net_method_idle_event2(
  tw_stime offset_ts, int is_recv_queue, int queue_offset, tw_lp* lp)
{
  tw_event* e = tw_event_new(lp->gid, offset_ts, lp);
  model_net_wrap_msg* m_wrap = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
  model_net_request* r_wrap = &m_wrap->msg.m_base.req;
#if DEBUG
  printf("%llu handle_sched_next() from model_net_method_idle_event2()\n", LLU(tw_now(lp)));
#endif
  msg_set_header(model_net_base_magic, MN_BASE_SCHED_NEXT, lp->gid, &m_wrap->h);
  m_wrap->msg.m_base.is_from_remote = is_recv_queue;
  r_wrap->queue_offset = queue_offset;
  tw_event_send(e);
}

void* model_net_method_get_edata(int net_id, void* msg)
{
  return (char*)msg + sizeof(model_net_wrap_msg) - msg_offsets[net_id];
}

// This is utilized by a workoad file, when a single workload rank has determined that
// all of the workload ranks have finished their respective tasks, it uses this method
// to send a notification to all model net LPs that the workloads have completed.
// This enables network models to implement features that require a heartbeat message
// that's repeated ad infinitum. If those ranks aren't notified that the workload
// ranks are completed, then the simulation will 'never' end. (it will run until g_tw_ts_end is
// reached) note: network models must explicitly implement and initialize the necessary
// model-net-method callback
// return number of random calls utilized by this method
int model_net_method_end_sim_broadcast(tw_stime offset_ts, tw_lp* sender)
{
  // get lp names of active model-net LPs
  // send end sim notification to each LP that match an active model-net lp name
  auto mapper = codes::Orchestrator::GetInstance().GetMapper();
  for (int cid = 0; cid < mapper->GetNumberUniqueLPTypes(); cid++)
  {
    auto lp_name = mapper->GetLPTypeNameByTypeId(cid);
    for (int n = 0; n < MAX_NETS; n++)
    {
      if (lp_name == model_net_lp_config_names[n])
      {
        // printf("ACTIVE TYPE FOUND: %s\n", lp_name);
        for (int lid = 0; lid < mapper->GetLPTypeCount(lp_name); lid++)
        {
          tw_lpid lpgid = mapper->GetLPIdFromRelativeId(lid, lp_name);
          tw_event* e = model_net_method_end_sim_notification(lpgid, offset_ts, sender);
          tw_event_send(e);
        }
      }
    }
  }
  return 0;
}

// see model_net_method_end_sim_broadcast for details on why this method is useful
tw_event* model_net_method_end_sim_notification(tw_lpid dest_gid, tw_stime offset_ts, tw_lp* sender)
{
  tw_event* e = tw_event_new(dest_gid, offset_ts, sender);
  model_net_wrap_msg* m_wrap = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
  msg_set_header(model_net_base_magic, MN_BASE_END_NOTIF, sender->gid, &m_wrap->h);
  return e;
}

tw_event* model_net_method_congestion_event(
  tw_lpid dest_gid, tw_stime offset_ts, tw_lp* sender, void** msg_data, void** extra_data)
{
  tw_event* e = tw_event_new(dest_gid, offset_ts, sender);
  model_net_wrap_msg* m_wrap = reinterpret_cast<model_net_wrap_msg*>(tw_event_data(e));
  msg_set_header(model_net_base_magic, MN_CONGESTION_EVENT, sender->gid, &m_wrap->h);
  *msg_data = ((char*)m_wrap) + msg_offsets[CONGESTION_CONTROLLER];
  // extra_data is optional
  if (extra_data != NULL)
  {
    *extra_data = m_wrap + 1;
  }
  return e;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=8 sts=4 sw=4 expandtab
 */
