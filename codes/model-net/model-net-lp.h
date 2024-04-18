/*
 * Copyright (C) 2014 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 */

/* This is the base model-net LP that all events pass through before
 * performing any topology-specific work. Packet scheduling, dealing with
 * packet loss (potentially), etc. happens here.
 * Additionally includes wrapper event "send" function that all
 * events for underlying models must go through */

#ifndef MODEL_NET_LP_H
#define MODEL_NET_LP_H

#include <ross.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "codes/mapping/codes-mapping.h"
#include "codes/model-net/congestion-controller-core.h"
#include "codes/model-net/lp-msg.h"
#include "codes/model-net/model-net-sched.h"
#include "codes/model-net/model-net.h"
#include "codes/model-net/simplenet-upd.h"
#include "codes/models/pdes/routers/simplep2p.h"

  extern int model_net_base_magic;

  typedef struct model_net_base_msg
  {
    // no need for event type - in wrap message
    model_net_request req;
    int is_from_remote;
    int isQueueReq;
    tw_stime save_ts;
    // parameters to pass to new messages (via model_net_set_msg_params)
    // TODO: make this a union for multiple types of parameters
    mn_sched_params sched_params;
    model_net_sched_rc rc; // rc for scheduling events
  } model_net_base_msg;

  typedef struct model_net_wrap_msg
  {
    msg_header h;
    union
    {
      model_net_base_msg m_base; // base lp
      sn_message m_snet;         // simplenet
      sp_message m_sp2p;         // simplep2p
      congestion_control_message m_cc;
      // add new ones here
    } msg;
  } model_net_wrap_msg;

  typedef struct model_net_base_params_s
  {
    model_net_sched_cfg_params sched_params;
    uint64_t packet_size;
    int num_queues;
    int use_recv_queue;
    tw_stime nic_seq_delay;
    int node_copy_queues;
  } model_net_base_params;

  typedef struct model_net_base_state
  {
    int net_id, nics_per_router;
    // whether scheduler loop is running
    int *in_sched_send_loop, in_sched_recv_loop;
    // unique message id counter. This doesn't get decremented on RC to prevent
    // optimistic orderings using "stale" ids
    uint64_t msg_id;
    // model-net schedulers
    model_net_sched **sched_send, *sched_recv;
    // parameters
    const model_net_base_params* params;
    // lp type and state of underlying model net method - cache here so we
    // don't have to constantly look up
    const tw_lptype* sub_type;
    const st_model_types* sub_model_type;
    void* sub_state;
    tw_stime next_available_time;
    tw_stime* node_copy_next_available_time;
  } model_net_base_state;

  /* ROSS LP processing functions */
  void model_net_base_lp_init(model_net_base_state* ns, tw_lp* lp);
  void model_net_base_event(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp);
  void model_net_base_event_rc(
    model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp);
  void model_net_base_finalize(model_net_base_state* ns, tw_lp* lp);

  /* event type handlers */
  void handle_new_msg(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp);
  void handle_sched_next(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp);
  void handle_new_msg_rc(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp);
  void handle_sched_next_rc(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp);
  void model_net_commit_event(model_net_base_state* ns, tw_bf* b, model_net_wrap_msg* m, tw_lp* lp);

  /* ROSS function pointer table for this LP */
  extern tw_lptype model_net_base_lp;

  // register the networks with ROSS, given the array of flags, one for each
  // network type
  void model_net_base_register(int* do_config_nets);
  // configure the base LP type, setting up general parameters
  void model_net_base_configure();
  void model_net_base_configure_yaml();

  /// The remaining functions/data structures are only of interest to model-net
  /// model developers

  // Construct a model-net-specific event, analagous to a tw_event_new and
  // codes_event_new. The difference here is that we return pointers to
  // both the message data (to be cast into the appropriate type) and the
  // pointer to the end of the event struct.
  //
  // This function is expected to be called within each specific model-net
  // method - strange and disturbing things will happen otherwise
  tw_event* model_net_method_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp* sender,
    int net_id, void** msg_data, void** extra_data);

  // Construct a model-net-specific event, similar to model_net_method_event_new.
  // The primary differences are:
  // - the event gets sent to final_dest_lp and put on it's receiver queue
  // - no message initialization is needed - that's the job of the
  //   model_net_method_recv_msg_event functions
  //
  // NOTE: this is largely a constructor of a model_net_request
  void model_net_method_send_msg_recv_event(tw_lpid final_dest_lp,
    tw_lpid dest_mn_lp, // which model-net lp is going to handle message
    tw_lpid src_lp,     // the "actual" source (as opposed to the model net lp)
    uint64_t msg_size,  // the size of this message
    int is_pull,
    uint64_t pull_size, // the size of the message to pull if is_pull==1
    int remote_event_size, const mn_sched_params* sched_params, const char* category, int net_id,
    void* msg, tw_stime offset, tw_lp* sender);
  // just need to reverse an RNG for the time being
  void model_net_method_send_msg_recv_event_rc(tw_lp* sender);

  // Issue an event from the underlying model (e.g., simplenet, loggp) to tell the
  // scheduler when next to issue a packet event. As different models update their
  // notion of "idleness" separately, this is necessary. DANGER: Failure to call
  // this function appropriately will cause the scheduler to hang or cause other
  // weird behavior.
  //
  // This function is expected to be called within each specific model-net
  // method - strange and disturbing things will happen otherwise
  void model_net_method_idle_event(tw_stime offset_ts, int is_recv_queue, tw_lp* lp);
  void model_net_method_idle_event2(
    tw_stime offset_ts, int is_recv_queue, int queue_offset, tw_lp* lp);

  // Get a ptr to past the message struct area, where the self/remote events
  // are located, given the type of network.
  // NOTE: this should ONLY be called on model-net implementations, nowhere else
  void* model_net_method_get_edata(int net_id, void* msg);

  int model_net_method_end_sim_broadcast(tw_stime offset_ts, tw_lp* sender);

  tw_event* model_net_method_end_sim_notification(
    tw_lpid dest_gid, tw_stime offset_ts, tw_lp* sender);

  // Wrapper for congestion controller to request congestion data from destination
  tw_event* model_net_method_congestion_event(
    tw_lpid dest_gid, tw_stime offset_ts, tw_lp* sender, void** msg_data, void** extra_data);

  /// The following functions/data structures should not need to be used by
  /// model developers - they are just provided so other internal components can
  /// use them

  enum model_net_base_event_type
  {
    MN_BASE_NEW_MSG,
    // schedule next packet
    MN_BASE_SCHED_NEXT,
    // gather a sample from the underlying model
    MN_BASE_SAMPLE,
    // message goes directly down to topology-specific event handler
    MN_BASE_PASS,
    /* message goes directly to topology-specific event handler for ending the simulation
       usefull if there is an infinite heartbeat pattern */
    MN_BASE_END_NOTIF,
    // message calls congestion request method on topology specific handler
    MN_CONGESTION_EVENT
  };

  void SetMsgOffsets(int i, int offset);
  void SetAnnos(int i, const char* anno);
  model_net_base_params* GetAllParams(int i);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: MODEL_NET_LP_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=8 sts=4 sw=4 expandtab
 */
