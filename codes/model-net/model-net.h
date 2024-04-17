/*
 * Copyright (C) 2013 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 */

#ifndef MODELNET_H
#define MODELNET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include <ross.h>

#include "codes/GlobalDefines.h"
#include "codes/config/configuration.h"
#include "codes/mapping/codes-mapping-context.h"
#include "codes/model-net/lp-io.h"
#include "codes/model-net/lp-type-lookup.h"

#ifdef ENABLE_CORTEX
#include <cortex/cortex.h>
#include <cortex/topology.h>
#endif

#define PULL_MSG_SIZE 128

#define MAX_NAME_LENGTH 256
#define CATEGORY_NAME_MAX 16
#define CATEGORY_MAX 15

// simple deprecation attribute hacking
#if !defined(DEPRECATED)
#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
#define DEPRECATED __attribute__((deprecated))
#else
#define DEPRECATED
#endif
#endif

  extern struct model_net_method simplenet_method;
  extern struct model_net_method simplep2p_method;
  extern struct model_net_method express_mesh_method;
  extern struct model_net_method express_mesh_router_method;
  /* Global declarations for configurations that might want to be passed via
   * command line in addition to configuration file. Needed a globally accessible
   * place for these
   */
  extern char g_nm_link_failure_filepath[]; // filepath to link failure file for use by models that
                                            // use NetworkManager

  /* HACK: there is currently no scheduling fidelity across multiple
   * model_net_event calls. Hence, problems arise when some LP sends multiple
   * messages as part of an event and expects FCFS ordering. A proper fix which
   * involves model-net LP-level scheduling of requests is ideal, but not
   * feasible for now (would basically have to redesign model-net), so expose
   * explicit start-sequence and stop-sequence markers as a workaround
   */
  extern int mn_in_sequence;
  extern tw_stime mn_msg_offset;
#define MN_START_SEQ()                                                                             \
  do                                                                                               \
  {                                                                                                \
    mn_in_sequence = 1;                                                                            \
    mn_msg_offset = 0.0;                                                                           \
  } while (0)
#define MN_END_SEQ()                                                                               \
  do                                                                                               \
  {                                                                                                \
    mn_in_sequence = 0;                                                                            \
  } while (0)

  typedef struct mn_stats mn_stats;

// TODO this needs to be rethought. we will potentially have a number of different models.
// So for one thing, for all of our ML models, say for a router we have a number of different
// configs. So there will be one router LP implemented in the code, and then that will load the
// appropriate pytorch model. so maybe we don't need to rework this for now I would like to do this
// better for making it easier in adding more types of lps in the future, but i think it's not super
// necessary to worry about yet
#define X(a, b, c, d) a,
  enum NETWORKS
  {
    NETWORK_DEF
  };
#undef X

  // message parameter types
  enum msg_param_type
  {
    // parameters for modelnet-scheduler (priorities and such)
    MN_MSG_PARAM_SCHED,
    // parameter allowing the explicit setting of a messages start time (useful
    // for routing scenarios in which a single message is implemented in terms
    // of multiple model-net events, such as for model_net_pull_*event's)
    MN_MSG_PARAM_START_TIME,
    MAX_MN_MSG_PARAM_TYPES
  };

  // MN_MSG_PARAM_MSG_START_TIME parameter types (only one for the time being)
  enum msg_param_start_time
  {
    MN_MSG_PARAM_START_TIME_VAL
  };

  // return type for model_net_*event calls, to be passed into RC
  // currently is just an int, but in the future may indicate whether lp-io was
  // called etc.
  typedef int model_net_event_return;

  // network identifiers (both the config lp names and the model-net internal
  // names)
  extern char* model_net_lp_config_names[];
  extern char* model_net_method_names[];

  // request structure that gets passed around (by the model-net implementation,
  // not the user)
  typedef struct model_net_request
  {
    tw_lpid final_dest_lp;
    tw_lpid dest_mn_lp; // destination modelnet lp
    tw_lpid src_lp;
    // time the source event was called
    tw_stime msg_start_time;
    uint64_t msg_size;
    uint64_t pull_size;
    uint64_t packet_size;
    // unique message id set, *at the modelnet LP*, for each message the
    // modelnet LP processes
    uint64_t msg_id;
    int net_id;
    int is_pull;
    int queue_offset;
    int remote_event_size;
    int self_event_size;
    char category[CATEGORY_NAME_MAX];

    // for counting msg app id
    int app_id;

  } model_net_request;

  /* data structure for tracking network statistics */
  struct mn_stats
  {
    char category[CATEGORY_NAME_MAX];
    long send_count;
    long send_bytes;
    tw_stime send_time;
    long recv_count;
    long recv_bytes;
    tw_stime recv_time;
    long max_event_size;
  };

  /* Registers all model-net LPs in ROSS. Should be called after
   * configuration_load, but before codes_mapping_setup */
  void model_net_register();
  // replaces above for new config
  void model_net_register_yaml();

  /* Configures all model-net LPs based on the CODES configuration, and returns
   * ids to address the different types by.
   *
   * id_count - the output number of networks
   *
   * return - the set of network IDs, indexed in the order given by the
   * modelnet_order configuration parameter */
  int* model_net_configure(int* id_count);
  int* model_net_configure_yaml(int* id_count);

  /* Sets up a sampling loop for model net events. The sampling data provided by
   * each modelnet lp is model-defined. This is a PE-wide setting. Data is sent
   * to LP-IO with the category modelnet-samples */
  void model_net_enable_sampling(tw_stime interval, tw_stime end);

  /* Returns 1 if modelnet is performing sampling, 0 otherwise */
  int model_net_sampling_enabled(void);

  /* utility function to get the modelnet ID post-setup */
  int model_net_get_id(char* net_name);

  /* This event does a collective operation call for model-net */
  void model_net_event_collective(int net_id, char const* category, int message_size,
    int remote_event_size, void const* remote_event, tw_lp* sender);

  /* reverse event of the collective operation call */
  void model_net_event_collective_rc(int net_id, int message_size, tw_lp* sender);

  /* allocate and transmit a new event that will pass through model_net to
   * arrive at its destination:
   *
   * - net_id: the type of network to send this message through. The set of
   *   net_id's is given by model_net_configure.
   * - category: category name to associate with this communication
   *   - OPTIONAL: callers can set this to NULL if they don't want to use it,
   *     and model_net methods can ignore it if they don't support it
   * - final_dest_lp: the LP that the message should be delivered to.
   *   - NOTE: this is _not_ the LP of an underlying network method (for
   *     example, it is not a torus or dragonfly LP), but rather the LP of an
   *     MPI process or storage server that you are transmitting to.
   * - message_size: this is the size of the message (in bytes) that modelnet
   *     will simulate transmitting to the final_dest_lp.  It can be any size
   *     (i.e. it is not constrained by transport packet size).
   * - remote_event_size: this is the size of the ROSS event structure that
   *     will be delivered to the final_dest_lp.
   * - remote_event: pointer to data to be used as the remote event message. When
   *   the message payload (of size message_size) has been fully delivered to the
   *   destination (given by final_dest_lp), this event will be issued.
   * - self_event_size: this is the size of the ROSS event structure that will
   *     be delivered to the calling LP once local completion has occurred for
   *     the network transmission.
   *     - NOTE: "local completion" in this sense means that model_net has
   *       transmitted the data off of the local node, but it does not mean that
   *       the data has been (or even will be) delivered.  Once this event is
   *       delivered the caller is free to re-use its buffer.
   * - self_event: pointer to data to be used as the self event message. When the
   *   message payload (of size message_size) has been fully sent from the
   *   sender's point of view (e.g. the corresponding NIC has sent out all
   *   packets for this message), the event will be issued to the sender.
   * - sender: pointer to the tw_lp structure of the API caller.  This is
   *     identical to the sender argument to tw_event_new().
   *
   * The modelnet LP used for communication is determined by the default CODES
   * map context (see codes-base, codes/codes-mapping-context.h), using net_id
   * to differentiate different model types. Note that the map context is used
   * when calculating *both* sender and receiver modelnet LPs
   */
  // first argument becomes the network ID
  model_net_event_return model_net_event(int net_id, char const* category, tw_lpid final_dest_lp,
    uint64_t message_size, tw_stime offset, int remote_event_size, void const* remote_event,
    int self_event_size, void const* self_event, tw_lp* sender);
  /*
   * See model_net_event for a general description.
   *
   * Unlike model_net_event, this function uses the annotation to differentiate
   * multiple modelnet LPs with the same type but different annotation. The caller
   * annotation is not consulted here. The corresponding CODES map context is
   * CODES_MCTX_GROUP_MODULO with the supplied annotation arguments.
   */
  model_net_event_return model_net_event_annotated(int net_id, char const* annotation,
    char const* category, tw_lpid final_dest_lp, uint64_t message_size, tw_stime offset,
    int remote_event_size, void const* remote_event, int self_event_size, void const* self_event,
    tw_lp* sender);
  /*
   * See model_net_event for a general description.
   *
   * This variant uses CODES map contexts to calculate the sender and receiver
   * modelnet LPs
   */
  model_net_event_return model_net_event_mctx(int net_id, struct codes_mctx const* send_map_ctx,
    struct codes_mctx const* recv_map_ctx, char const* category, tw_lpid final_dest_lp,
    uint64_t message_size, tw_stime offset, int remote_event_size, void const* remote_event,
    int self_event_size, void const* self_event, tw_lp* sender);

  /* model_net_find_local_device()
   *
   * returns the LP id of the network card attached to the calling LP using the
   * default CODES mapping context if ignore_annotations is true, and
   * CODES_MCTX_GROUP_MODULO with the supplied annotation parameters otherwise
   */
  tw_lpid model_net_find_local_device(
    int net_id, const char* annotation, int ignore_annotations, tw_lpid sender_gid);

  /* same as ^, except use the supplied mapping context */
  tw_lpid model_net_find_local_device_mctx(
    int net_id, struct codes_mctx const* map_ctx, tw_lpid sender_gid);

  int model_net_get_msg_sz(int net_id);

  /* model_net_event_rc()
   *
   * This function does reverse computation for the model_net_event_new()
   * function.
   * - sender: pointer to the tw_lp structure of the API caller.  This is
   *   identical to the sender argument to tw_event_new().
   */
  /* NOTE: we may end up needing additoinal arguments here to track state for
   * reverse computation; add as needed
   */
  DEPRECATED
  void model_net_event_rc(int net_id, tw_lp* sender, uint64_t message_size);

  /* This function replaces model_net_event_rc, and will replace the name later
   * on. The num_rng_calls argument is the return value of model_net_*event*
   * calls */
  void model_net_event_rc2(tw_lp* sender, model_net_event_return const* ret);

  /* Issue a 'pull' from the memory of the destination LP, without
   * requiring the destination LP to do event processing. This is meant as a
   * simulation-based abstraction of RDMA. A control packet will be sent to the
   * destination LP, the payload will be sent back to the requesting LP, and the
   * requesting LP will be issued it's given completion event.
   *
   * Parameters are largely the same as model_net_event, with the following
   * exceptions:
   * - final_dest_lp is the lp to pull data from
   * - self_event_size, self_event are applied at the requester upon receipt of
   *   the payload from the dest
   */
  model_net_event_return model_net_pull_event(int net_id, char const* category,
    tw_lpid final_dest_lp, uint64_t message_size, tw_stime offset, int self_event_size,
    void const* self_event, tw_lp* sender);
  model_net_event_return model_net_pull_event_annotated(int net_id, char const* annotation,
    char const* category, tw_lpid final_dest_lp, uint64_t message_size, tw_stime offset,
    int self_event_size, void const* self_event, tw_lp* sender);
  model_net_event_return model_net_pull_event_mctx(int net_id,
    struct codes_mctx const* send_map_ctx, struct codes_mctx const* recv_map_ctx,
    char const* category, tw_lpid final_dest_lp, uint64_t message_size, tw_stime offset,
    int self_event_size, void const* self_event, tw_lp* sender);

  DEPRECATED
  void model_net_pull_event_rc(int net_id, tw_lp* sender);

  /*
   * Set message-specific parameters
   * type     - overall type (see msg_param_type)
   * sub_type - type of parameter specific to type. This is intended to be
   *            an enum for each of msg_param_type's values
   * params   - the parameter payload
   *
   * This function works by setting up a temporary parameter context within the
   * model-net implementation (currently implemented as a set of translation-unit
   * globals). Upon a subsequent model_net_*event* call, the context is consumed
   * and reset to an unused state.
   *
   * NOTE: this call MUST be placed in the same calling context as the subsequent
   * model_net_*event* call. Otherwise, the parameters are not guaranteed to work
   * on the intended event, and may possibly be consumed by another, unrelated
   * event.
   */
  void model_net_set_msg_param(enum msg_param_type type, int sub_type, const void* params);

  /* returns pointer to LP information for simplenet module */
  const tw_lptype* model_net_get_lp_type(int net_id);
  const st_model_types* model_net_get_model_stat_type(int net_id);

  DEPRECATED
  uint64_t model_net_get_packet_size(int net_id);

  /* used for reporting overall network statistics for e.g. average latency ,
   * maximum latency, total number of packets finished during the entire
   * simulation etc. */
  void model_net_report_stats(int net_id);

  /* writing model-net statistics on a per LP basis */
  void model_net_write_stats(tw_lpid lpid, mn_stats* stat);

  /* printing model-net statistics on a per LP basis */
  void model_net_print_stats(tw_lpid lpid, mn_stats mn_stats_array[]);

  /* find model-net statistics */
  mn_stats* model_net_find_stats(char const* category, mn_stats mn_stats_array[]);

  extern double cn_bandwidth;
  extern tw_stime codes_cn_delay;
  extern int codes_node_eager_limit;
  void initMsgParamsSet();

#ifdef ENABLE_CORTEX
  /* structure that gives access to the topology functions */
  extern cortex_topology model_net_topology;
#endif

#ifdef __cplusplus
}
#endif

#endif /* MODELNET_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=8 sts=4 sw=4 expandtab
 */
