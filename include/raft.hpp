#pragma once
#include <string>
#include "proto_gen/raft.pb.h"
#include "http_rpc.h"
#include "common.hpp"
#include "log.hpp"
#include "log_manager.h"
#include "mmap_log.hpp"
#include "snapshot.h"
#include "log_compaction.h"
#include "election_timer.h"
#include "peer.h"
#include "node.h"
#include "node_service.h"
#include "server.hpp"

/*  raft paper https://raft.github.io/raft.pdf
 *
 *
 */