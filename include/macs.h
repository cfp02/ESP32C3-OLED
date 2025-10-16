#pragma once

#include <stddef.h>


//List of mac addresses with names
struct HoverboardMAC {
    const char* name;
    const char* address;
};

static const HoverboardMAC HOVERBOARD_MACS[] = {
    {"H1", "AC:67:B2:53:86:28"},
    {"H2", "E8:DB:84:03:F1:80"}
};


static const size_t NUM_HOVERBOARDS = sizeof(HOVERBOARD_MACS) / sizeof(HOVERBOARD_MACS[0]);
