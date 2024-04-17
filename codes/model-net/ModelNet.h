//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_MODEL_NET_H
#define CODES_MODEL_NET_H

// okay there's something weird with building, where if ross isn't included first
// somehow there's some error about conflicting types for Init in mpicxx
// there's definitely some issue with how MPI includes are handled in the cmake
// build, so it's probably related to that
#include <ross.h>

#include "codes/GlobalDefines.h"
#include "codes/model-net/model-net-lp.h"
#include "codes/orchestrator/CodesYAML.h"
// #include "codes/model-net/model-net.h"

#include <string>

namespace codes
{

/**
 * Singleton class that sets up the model-net layer
 */
class ModelNet
{
public:
#define X(a, b, c, d) a,
  enum Networks
  {
    NETWORK_DEF
  };
#undef X

  // message parameter types
  enum class MessageParamType
  {
    // parameters for modelnet-scheduler (priorities and such)
    ParamSched,
    // parameter allowing the explicit setting of a messages start time (useful
    // for routing scenarios in which a single message is implemented in terms
    // of multiple model-net events, such as for model_net_pull_*event's)
    ParamStartTime,
    MaxParamTypes
  };

  // MN_MSG_PARAM_MSG_START_TIME parameter types (only one for the time being)
  enum class MessageParamStartTime
  {
    ParamStartTimeValue
  };

  static ModelNet& GetInstance();

  /* Registers all model-net LPs in ROSS. Should be called after
   * configuration_load, but before codes_mapping_setup */
  void Register();

  /* Configures all model-net LPs based on the CODES configuration, and returns
   * ids to address the different types by.
   *
   * id_count - the output number of networks
   *
   * return - the set of network IDs, indexed in the order given by the
   * modelnet_order configuration parameter */
  std::vector<int> Configure(int& idCount);

  /* Initialize/configure the network(s) based on the CODES configuration.
   * returns an array of the network ids, indexed in the order given by the
   * modelnet_order configuration parameter
   * OUTPUT id_count - the output number of networks */
  int* SetParams(int* id_count);

  /* utility function to get the modelnet ID post-setup */
  int GetId(const std::string& net_name);

private:
  ModelNet();
  ModelNet(const ModelNet&) = delete;
  ModelNet& operator=(const ModelNet&) = delete;
  ~ModelNet();

  static void CreateInstance();

  static ModelNet* Instance;
  static bool Destroyed;

  std::string LinkFailureFilePath;

  std::shared_ptr<orchestrator::CodesYAML> YAMLParser;
  std::vector<int> ConfiguredNetworks;

  void BaseConfigure();
};

// request structure that gets passed around (by the model-net implementation,
// not the user)
struct model_net_request
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
};

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

extern "C"
{

  /* This event does a collective operation call for model-net */
  void mn_event_collective(int net_id, char const* category, int message_size,
    int remote_event_size, void const* remote_event, tw_lp* sender);

  /* reverse event of the collective operation call */
  void mn_event_collective_rc(int net_id, int message_size, tw_lp* sender);

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
   * - remcte_event: pointer to data to be used as the remote event message. When
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
  // mn_event_return model_net_event(int net_id, char const* category, tw_lpid final_dest_lp,
  //  uint64_t message_size, tw_stime offset, int remote_event_size, void const* remote_event,
  //  int self_event_size, void const* self_event, tw_lp* sender);
};

} // end namespace codes

#endif // CODES_MODEL_NET_H
