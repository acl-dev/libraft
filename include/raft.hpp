#pragma once
#include <string>
#ifndef _WIN32
#include<sys/mman.h> //mmap
#endif
#include "proto_gen/raft.pb.h"
#include "http_rpc.h"
#include "common.hpp"
#include "log.hpp"
#include "log_manager.h"
#include "mmap_log.hpp"
#include "peer.h"
#include "metadata.h"
#include "node.h"

/*  raft paper https://raft.github.io/raft.pdf
 *
 *
 *
 *
 *
 *
 */