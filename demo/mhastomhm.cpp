/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2017 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

// System includes
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

// External includes
#include "ilo/memory.h"

// Internal includes
#include "common.h"
#include "logging.h"
#include "mmtmhasparserlib/mhaspacketizer.h"
#include "mmtmhasparserlib/mhasasipacket.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"

using namespace mmt::mhasparserlib;

static void readCompleteFileToBuffer(const std::string& fileName, ilo::ByteBuffer& buffer) {
  std::ifstream file(fileName, std::ios_base::binary | std::ios_base::in);
  if (!file) {
    throw std::runtime_error{"Unable to open input file"};
  }

  file.seekg(0, std::ios_base::end);
  auto size = file.tellg();
  file.seekg(0, std::ios_base::beg);

  buffer.resize(static_cast<std::size_t>(size));
  file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));
}

static bool isSubstreamFrame(const CUniqueMhasPacket& packet) {
  // packet labels larger than 16 are used for the sub-streams (ISO/IEC 23008-3,
  // section 14.6)
  return packet &&
         static_cast<EMhasPacketType>(packet->packetType()) ==
             EMhasPacketType::PACTYP_MPEGH3DAFRAME &&
         packet->packetLabel() > 16;
}

static bool isMultistream(const SMhasAccessUnit& accessUnit) {
  // Easy case, there are sub-stream audio frames in the access unit (i.e. merged multi-stream)
  if (std::any_of(accessUnit.packets.begin(), accessUnit.packets.end(), isSubstreamFrame)) {
    return true;
  }
  // Expert case, the ASI for the main stream indicates existence of additional streams not actually
  // present in the MHAS input
  const CMhasConfigPacket* configPacket = nullptr;
  const CMhasAsiPacket* asiPacket = nullptr;
  for (const auto& packet : accessUnit.packets) {
    if (packet->packetLabel() == 0) {
      continue;
    }
    if (const auto* config = dynamic_cast<const CMhasConfigPacket*>(packet.get())) {
      configPacket = config;
    } else if (const auto* asi = dynamic_cast<const CMhasAsiPacket*>(packet.get())) {
      asiPacket = asi;
    }
  }
  if (!configPacket || !asiPacket) {
    return false;
  }

  // The total number of IDs is larger than the number of signals in the MPEG-H 3DA main stream,
  // thus assume there are associated MPEG-H 3DA sub-streams.
  return configPacket->mhasConfigInfo().signals3d.signals.size() <
         (asiPacket->audioSceneInfo().metaDataElementIDmaxAvail + 1U /* ID 0 */);
}

struct SBitstreamConfig {
  uint32_t outputSampleRate = 0;
  uint32_t frameSize = 0;
};

static void printHelp() {
  std::cout << "Usage: <mhastomhm> -o <output MP4 file> <input MHAS file>" << std::endl;
  std::cout << std::endl;
  std::cout << "Parameter:" << std::endl;
  std::cout << "  -h, --help           Show this help and exit." << std::endl;
  std::cout << "  -o <output MP4 file> The output file to write the generated MP4 contents to."
            << std::endl;
  std::cout << "  <input MHAS file>    The input file to read the MPEG-H MHAS stream from."
            << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc != 2 && argc != 4) {
    // All parameters are mandatory
    printHelp();
    return EXIT_FAILURE;
  }

  // Redirect mmtisobmff logs to syslog
  LOG_REDIRECT_TO_SYSTEM_LOG();

  std::string inputFileName{};
  std::string outputFileName{};

  for (int i = 1; i < argc; ++i) {
    if (std::string{"-h"} == argv[i] || std::string{"--help"} == argv[i]) {
      printHelp();
      return EXIT_SUCCESS;
    } else if (std::string{"-o"} == argv[i]) {
      if ((i + 1) >= argc) {
        printHelp();
        return EXIT_FAILURE;
      }
      outputFileName = argv[i + 1];
      ++i;
    } else if (i == argc - 1) {
      inputFileName = argv[i];
    } else {
      printHelp();
      return EXIT_FAILURE;
    }
  }

  if (outputFileName.empty()) {
    std::cout << "No output MP4 file specified!\n" << std::endl;
    printHelp();
    return EXIT_FAILURE;
  }

  try {
    ilo::ByteBuffer fileContent, outputBuffer;
    readCompleteFileToBuffer(inputFileName, fileContent);

    CMhasPacketizer::SConfig config{};
    config.strictMode = true;
    CMhasPacketizer packetizer{config};
    packetizer.feed(fileContent);
    packetizer.parseAccessUnits();

    auto accessUnits = packetizer.allAvailableAccessUnits();
    if (accessUnits.empty()) {
      std::cout << "No access units found, aborting" << std::endl;
      return EXIT_SUCCESS;
    }

    if (!accessUnits.front() || !accessUnits.front().sampleRate) {
      throw std::runtime_error{"There has to be a config before the first frame."};
    }

    CMhmOutput mp4output{outputFileName, accessUnits.front().sampleRate,
                         std::any_of(accessUnits.begin(), accessUnits.end(), isMultistream)};

    for (auto& au : accessUnits) {
      mp4output.writeSample(std::move(au.packets), au.duration, au.isIpf);
    }

  } catch (const std::exception& e) {
    std::cout << "Caught exception: " << e.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
