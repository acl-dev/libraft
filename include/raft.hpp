#pragma once
#include <string>
#include "proto_gen/raft.pb.h"
#include "http_rpc.h"
#include "common.hpp"
#include "log.hpp"
#include "log_manager.h"
#include "mmap_log.hpp"
#include "peer.h"
#include "node.h"

/*  raft paper https://raft.github.io/raft.pdf
 *
 *
 *
 *
 *
 *
 */