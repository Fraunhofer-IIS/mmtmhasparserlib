/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2019 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "mmtmhasparserlib/mhasparser.h"
#include "mmtaudioparser/mpeghparser.h"

using namespace mmt::audioparser;
using namespace mmt::mhasparserlib;

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "usage: configParser <file.mhas>" << std::endl;
    return -1;
  }

  std::string inputFile = argv[1];

  std::cout << "Parsing input file: " << inputFile << std::endl;
  std::ifstream inStream(inputFile, std::ios_base::binary | std::ios_base::in);

  if (!inStream) {
    std::cout << "Error opening input file: " << inputFile << std::endl;
    return -1;
  }

  std::map<uint64_t, uint16_t> crc16map;
  CMhasParser mhasParser;
  mhasParser.sync();

  ilo::ByteBuffer buffer(8192);

  std::vector<std::unique_ptr<ilo::ByteBuffer>> prevConfigs;
  std::size_t configCount = 0;
  ilo::ByteBuffer currentConfig;

  while (!inStream.eof()) {
    // Read from input file
    inStream.read(reinterpret_cast<char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size()));
    buffer.resize(static_cast<size_t>(inStream.gcount()));

    // Feed the MHAS parser
    mhasParser.feed(buffer);
    mhasParser.parsePackets();

    while (CUniqueMhasPacket mhasPacket = mhasParser.nextPacket()) {
      switch (EMhasPacketType(mhasPacket->packetType())) {
        case EMhasPacketType::PACTYP_MPEGH3DACFG:
          try {
            if (mhasPacket->payload() != currentConfig) {
              CMpeghParser parser;
              parser.addConfig(mhasPacket->payload());
              auto info = parser.getConfigInfo();

              std::cout << "Detected new config #: " << ++configCount << std::endl;
              std::cout << std::endl;

              std::cout << "Parsed config parameters:" << std::endl;

              std::cout << "mpegh3daProfileLevelIndicator: "
                        << std::to_string(info.profileLevelIndicator) << std::endl;
              std::cout << "usacSamplingFrequency: " << std::to_string(info.samplingFrequency)
                        << std::endl;
              std::cout << "coreSbrFrameLengthIndex: "
                        << std::to_string(info.coreSbrFrameLengthIndex) << std::endl;
              std::cout << "cfg_reserved: " << std::to_string(info.cfg_reserved) << std::endl;
              std::cout << "receiverDelayCompensation: "
                        << std::to_string(info.receiverDelayCompensation) << std::endl;
              std::cout << std::endl;
              std::cout << "reference Layout SpeakerLayoutType: "
                        << std::to_string(info.referenceLayout.speakerLayoutType) << std::endl;
              if (info.referenceLayout.speakerLayoutType == 0) {
                std::cout << "reference Layout CICP Idx: "
                          << std::to_string(info.referenceLayout.CICPIdx) << std::endl;
              } else {
                std::cout << "reference Layout num Speakers: "
                          << std::to_string(info.referenceLayout.numSpeakers) << std::endl;
              }
              std::cout << std::endl;
              std::cout << "num Signal Groups: " << std::to_string(info.signalGroups.size())
                        << std::endl;
              std::cout << "num Audio Channels: " << std::to_string(info.numAudioChannels)
                        << std::endl;
              std::cout << "num Audio Objects: " << std::to_string(info.numAudioObjects)
                        << std::endl;
              std::cout << "num SAOC Transport Channels: "
                        << std::to_string(info.numSAOCTransportChannels) << std::endl;
              std::cout << "num HOA Transport Channels: "
                        << std::to_string(info.numHOATransportChannels) << std::endl;
              std::cout << std::endl;
              std::cout << "num Elements: " << std::to_string(info.elementConfigs.size())
                        << std::endl;
              for (const auto& elementConfig : info.elementConfigs) {
                std::cout << "Element ID: " << elementConfig.usacElementType;
                switch (elementConfig.usacElementType) {
                  case 0:
                    std::cout << " (SCE)" << std::endl;
                    break;
                  case 1:
                    std::cout << " (CPE)" << std::endl;
                    break;
                  case 2:
                    std::cout << " (LFE)" << std::endl;
                    break;
                  case 3:
                    std::cout << " (EXT) with ID: " << elementConfig.extElementType << std::endl;
                    break;
                }
              }
              std::cout << std::endl;
              std::cout << "usacConfigExtensionPresent: "
                        << std::to_string(!info.configExtensions.empty()) << std::endl;
              for (CMpeghParser::SConfigExtension configInfo : info.configExtensions) {
                std::cout << "Extension ID " << configInfo.usacConfigExtType
                          << " has the configLength " << configInfo.usacConfigExtLength
                          << std::endl;
              }

              std::cout << std::endl;
              std::cout << std::endl;

              currentConfig = mhasPacket->payload();
            }
          } catch (const std::exception& e) {
            std::cout << std::endl << "ERROR: " << e.what() << std::endl;
            return 1;
          } catch (...) {
            std::cout << std::endl
                      << "ERROR: An unknown error happened. The program will exit now."
                      << std::endl;
            return 1;
          }
          break;
        default:
          // search for config package
          continue;
      }
    }
  }
  return 0;
}
