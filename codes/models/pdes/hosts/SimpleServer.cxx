//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/models/pdes/hosts/SimpleServer.h"
#include "codes/mapping/Mapper.h"
#include "codes/orchestrator/Orchestrator.h"
#include "codes/util/CodesUtils.h"

namespace
{

static int NETWORK_ID = 0;

} // end anon namespace

void SimpleServerSetNetId(int net_id)
{
  NETWORK_ID = net_id;
}

void simple_server_init(SimpleServerState* ns, tw_lp* lp);
void simple_server_event(SimpleServerState* ns, tw_bf* b, SimpleServerMsg* m, tw_lp* lp);
void svr_rev_event(SimpleServerState* ns, tw_bf* b, SimpleServerMsg* m, tw_lp* lp);
void svr_finalize(SimpleServerState* ns, tw_lp* lp);
static void handle_kickoff_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp);
static void handle_ack_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp);
static void handle_req_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp);
static void handle_local_event(SimpleServerState* ns);
static void handle_local_rev_event(SimpleServerState* ns);
static void handle_kickoff_rev_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp);
static void handle_ack_rev_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp);
static void handle_req_rev_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp);

tw_lptype SimpleServerLP = {
  (init_f)simple_server_init,
  (pre_run_f)NULL,
  (event_f)simple_server_event,
  (revent_f)svr_rev_event,
  (commit_f)NULL,
  (final_f)svr_finalize,
  (map_f)codes::CodesMapping,
  sizeof(SimpleServerState),
};

// TODO have the Orchestrator be able to call this and you pass to it the lp name and
// the tw_lptype struct
void SimpleServerAddLPType()
{
  lp_type_register("SimpleServer", &SimpleServerLP);
  codes::orchestrator::Orchestrator::GetInstance().LPTypeRegister("SimpleServer", &SimpleServerLP);
}

void simple_server_init(SimpleServerState* ns, tw_lp* lp)
{
  tw_event* e;
  SimpleServerMsg* m;
  tw_stime kickoff_time;

  memset(ns, 0, sizeof(*ns));

  /* each server sends a dummy event to itself that will kick off the real
   * simulation
   */

  // printf("\n Initializing servers %d ", (int)lp->gid);
  /* skew each kickoff event slightly to help avoid event ties later on */
  kickoff_time = g_tw_lookahead + tw_rand_unif(lp->rng);

  e = tw_event_new(lp->gid, kickoff_time, lp);
  m = reinterpret_cast<SimpleServerMsg*>(tw_event_data(e));
  m->EventType = SimpleServerEventTypes::KICKOFF;
  tw_event_send(e);

  return;
}

void simple_server_event(SimpleServerState* ns, tw_bf* b, SimpleServerMsg* m, tw_lp* lp)
{
  (void)b;
  switch (m->EventType)
  {
    case SimpleServerEventTypes::REQ:
      handle_req_event(ns, m, lp);
      break;
    case SimpleServerEventTypes::ACK:
      handle_ack_event(ns, m, lp);
      break;
    case SimpleServerEventTypes::KICKOFF:
      handle_kickoff_event(ns, m, lp);
      break;
    case SimpleServerEventTypes::LOCAL:
      handle_local_event(ns);
      break;
    default:
      printf("\n Invalid message type %d ", m->EventType);
      assert(0);
      break;
  }
}

void svr_rev_event(SimpleServerState* ns, tw_bf* b, SimpleServerMsg* m, tw_lp* lp)
{
  (void)b;
  switch (m->EventType)
  {
    case SimpleServerEventTypes::REQ:
      handle_req_rev_event(ns, m, lp);
      break;
    case SimpleServerEventTypes::ACK:
      handle_ack_rev_event(ns, m, lp);
      break;
    case SimpleServerEventTypes::KICKOFF:
      handle_kickoff_rev_event(ns, m, lp);
      break;
    case SimpleServerEventTypes::LOCAL:
      handle_local_rev_event(ns);
      break;
    default:
      assert(0);
      break;
  }

  return;
}

void svr_finalize(SimpleServerState* ns, tw_lp* lp)
{
  double t = codes::NSToSeconds(tw_now(lp) - ns->start_ts);
  printf("server %llu recvd %d bytes in %f seconds, %f MiB/s sent_count %d recvd_count %d "
         "local_count %d \n",
    (unsigned long long)lp->gid, PAYLOAD_SZ * ns->msg_recvd_count, t,
    ((double)(PAYLOAD_SZ * NUM_REQS) / (double)(1024 * 1024) / t), ns->msg_sent_count,
    ns->msg_recvd_count, ns->local_recvd_count);
  return;
}

/* handle initial event */
static void handle_kickoff_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp)
{
  SimpleServerMsg m_local, m_remote;

  //    m_local.EventType = SimpleServerEventTypes::REQ;
  m_local.EventType = SimpleServerEventTypes::LOCAL;
  m_local.src = lp->gid;

  memcpy(&m_remote, &m_local, sizeof(SimpleServerMsg));
  m_remote.EventType = SimpleServerEventTypes::REQ;
  // printf("handle_kickoff_event(), lp %llu.\n", (unsigned long long)lp->gid);

  /* record when transfers started on this server */
  ns->start_ts = tw_now(lp);

  /* each server sends a request to the next highest server */
  int dest_id;
  switch (lp->gid / 2)
  {
    case 0:
      dest_id = 4;
      break;
    case 1:
      dest_id = 4;
      break;
    case 2:
      return; /* LP 4 doesn't send messages */
  }
  m->ret = model_net_event(NETWORK_ID, "test", dest_id, PAYLOAD_SZ, 0.0, sizeof(SimpleServerMsg),
    &m_remote, sizeof(SimpleServerMsg), &m_local, lp);
  ns->msg_sent_count++;
}

static void handle_local_event(SimpleServerState* ns)
{
  ns->local_recvd_count++;
}

static void handle_local_rev_event(SimpleServerState* ns)
{
  ns->local_recvd_count--;
}
/* reverse handler for req event */
static void handle_req_rev_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp)
{
  ns->msg_recvd_count--;
  model_net_event_rc2(lp, &m->ret);

  return;
}

/* reverse handler for kickoff */
static void handle_kickoff_rev_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp)
{
  ns->msg_sent_count--;
  model_net_event_rc2(lp, &m->ret);

  return;
}

/* reverse handler for ack*/
static void handle_ack_rev_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp)
{
  if (m->incremented_flag)
  {
    model_net_event_rc2(lp, &m->ret);
    ns->msg_sent_count--;
  }
  return;
}

/* handle recving ack */
static void handle_ack_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp)
{
  SimpleServerMsg m_local;
  SimpleServerMsg m_remote;

  m_local.EventType = SimpleServerEventTypes::LOCAL;
  m_local.src = lp->gid;

  memcpy(&m_remote, &m_local, sizeof(SimpleServerMsg));
  m_remote.EventType = SimpleServerEventTypes::REQ;

  if (ns->msg_sent_count < NUM_REQS)
  {
    m->ret = model_net_event(NETWORK_ID, "test", m->src, PAYLOAD_SZ, 0.0, sizeof(SimpleServerMsg),
      &m_remote, sizeof(SimpleServerMsg), &m_local, lp);
    ns->msg_sent_count++;
    m->incremented_flag = 1;
  }
  else
  {
    m->incremented_flag = 0;
  }

  return;
}

/* handle receiving request */
static void handle_req_event(SimpleServerState* ns, SimpleServerMsg* m, tw_lp* lp)
{
  SimpleServerMsg m_local;
  SimpleServerMsg m_remote;

  m_local.EventType = SimpleServerEventTypes::LOCAL;
  m_local.src = lp->gid;

  memcpy(&m_remote, &m_local, sizeof(SimpleServerMsg));
  m_remote.EventType = SimpleServerEventTypes::ACK;

  ns->msg_recvd_count++;

  // mm Q: What should be the size of an ack message? may be a few bytes? or larger..?
  m->ret = model_net_event(NETWORK_ID, "test", m->src, PAYLOAD_SZ, 0.0, sizeof(SimpleServerMsg),
    &m_remote, sizeof(SimpleServerMsg), &m_local, lp);
}
