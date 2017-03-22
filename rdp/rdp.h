/**
 * @author ejrbuss
 * @date 2017
 */
#ifndef RDP_RDP_HEADER
#define RDP_RDP_HEADER

// Constants
const char* rdp_version = "0.0.1";
const char* rdp_logo = "\n"
    "  _____   _____   _____\n"
    " |     | |     \\ |     |\n"
    " |     \\ |      \\|    _|\n"
    " |__|\\__\\|______/|___|\n"
    "Reliable Datagram Protocol\n"
    "               @author Eric Buss\n"
    "               @version 0.0.1\n\n";

// Headers
#include "filestream.h"
#include "netconfig.h"
#include "receiver.h"
#include "sender.h"
#include "util.h"

#endif