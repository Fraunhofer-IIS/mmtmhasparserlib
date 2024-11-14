# Main Page {#mainpage}

## Description

The MHAS parser library can parse MHAS streams from raw input buffers and extract various types of MHAS packets from the stream.

## Minimal Code Example

The following lines provide a simple example showing how to extract MHAS packets from a raw binary buffers.

```{.cpp}
#include "mmtmhasparserlib/mhasparser.h"

using namespace mmt::mhasparserlib;

...

// e.g. read from a file or network stream
ilo::ByteBuffer rawData = ...;

// initialize the parser
CMhasParser parser{};

// feed the raw bytes into the parser
parser.feed(rawData);

// parse all full packets in the internal buffers
parser.parsePackets();

// check whether a sync packet was found
bool synced = parser.isSynced();

// check the number of MHAS packets that can be extracted
auto numPackets = numPacketsAvailable();

// extract all parsed packets
auto packets = parser.allAvailablePackets();

// do something with the packets...
...

```
