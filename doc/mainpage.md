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
auto numPackets = parser.numPacketsAvailable();

// extract all parsed packets
auto packets = parser.allAvailablePackets();

// do something with the packets...
...

```

## Additional parsers

The library provides some additional APIs for related use-cases:

The `CMhasPacketizer` (header `mmtmhasparserlib/mhaspacketizer.h`) parses MHAS packets and groups them into Access Units.
Additionally, the MHAS packetizer supports parsing (and grouping) of multistream MHAS as well as functionality to extract the
MHAS packets for the single contained streams (e.g. to split a multistream MHAS).
Only Low Complexity and Baseline multistream MHAS inputs with identical sample rates, frame sizes of 1024 samples and identical truncation durations for corresponding frames are supported.

The `CPesParser` (header `mmtmhasparserlib/pesparser.h`) parses MHAS packets from an MPEG-2 TS PES (Packetized Elementary Stream) and groups them into Access Units.
Additionally, the PES parser augments the parsed MHAS Access Units with timestamp information extracted from the encapsulating PES stream.
Multistream MHAS or non-MHAS payload in the input PES is not supported.


## Extracting MHAS from MPEG-2 TS Streams

The `pestomhm` tool can convert an MPEG-2 TS PES file to an MP4 file containing an MPEG-H audio track. This tool can be enabled via the `mmtmhasparserlib_BUILD_BINARIES` CMake option. In combination with an MPEG-2 TS processing tool capable of extracting an MPEG-2 TS PES from an MPEG-2 Transport Stream, the `pestomhm` demo tool can be used to extract an MHAS Elementary Stream from the MPEG-2 TS Stream and convert it to an MP4 file containing an MPEG-H audio track (mhm1/mhm2).

One MPEG-2 TS processing tool capable of extracting PES streams is [TSDuck](https://tsduck.io/). A command line to extract an MHAS PES and convert it to MP4 could for example look like this:

    tsp -I file --format TS <input TS file> -P pes --save-pes - -O drop | ./bin/pestomhm -o <output MP4 file> /dev/stdin
