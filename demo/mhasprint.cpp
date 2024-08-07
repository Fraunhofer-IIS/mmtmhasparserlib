/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2020 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
#include <string>

// Internal includes
#include "mmtmhasparserlib/mhasinfowrapper.h"

using namespace mmt::mhasparserlib;

static const std::map<uint8_t, std::string> CICP_NAME{
    {uint8_t(1), "Mono"},        {uint8_t(2), "2.0"},          {uint8_t(3), "3.0"},
    {uint8_t(4), "LRCS"},        {uint8_t(5), "5.0"},          {uint8_t(6), "5.1"},
    {uint8_t(7), "7.1 (5 / 2)"}, {uint8_t(9), "3.0 Surr"},     {uint8_t(10), "Quad"},
    {uint8_t(11), "5.1 + Back"}, {uint8_t(12), "7.1 (3 / 4)"}, {uint8_t(13), "22.2"},
    {uint8_t(14), "5.1 + 2H"},   {uint8_t(15), "7.2 + 3H"},    {uint8_t(16), "5.1 + 4H"},
    {uint8_t(17), "5.1 + 6H"},   {uint8_t(18), "7.1 + 6H"},    {uint8_t(19), "7.1 + 4H"},
    {uint8_t(20), "9.1 + 4H"}};

static std::string cicpToStr(uint8_t cicpSpeakerLayoutIdx) {
  std::string ret;
  if (CICP_NAME.count(cicpSpeakerLayoutIdx) != 0) {
    ret = CICP_NAME.at(cicpSpeakerLayoutIdx);
  } else {
    ret = "Unknown CICP index";
  }

  ret += " (CICP Speaker Layout index = " + std::to_string(cicpSpeakerLayoutIdx) + ")";
  return ret;
}

static void printUsage() {
  std::cout << "Usage: mhasprint <path to mhas file>" << std::endl;
}

template <std::size_t SIZE>
static std::string langcodeToStr(const std::array<char, SIZE>& lang) {
  return std::string(lang.data(), lang.size());
}

static void printItemDescriptions(
    const std::vector<CMhasInfoWrapper::SItemDescription>& descriptions) {
  for (size_t i = 0; i < descriptions.size(); i++) {
    std::string append;
    if (descriptions.size() > 1) {
      append = " (" + std::to_string(i) + ")";
    }

    std::cout << "   Description" << append << ": " << descriptions[i].desc << std::endl;
    std::cout << "   Language" << append << ": " << langcodeToStr(descriptions[i].lang)
              << std::endl;
  }
}

static void printSpeakerConfig3d(const CMhasConfigPacket::SSpeakerConfig3d& speakerConfig,
                                 const std::string& prefix = "") {
  std::cout << prefix << "Speaker Config Type: ";
  switch (speakerConfig.speakerLayoutType) {
    case CMhasConfigPacket::SSpeakerConfig3d::ESpeakerLayoutType::CICPSPEAKERLAYOUTIDX:
      std::cout << "CICP Speaker Layout Index" << std::endl;
      std::cout << prefix << "Speaker Layout: " << cicpToStr(speakerConfig.cicpSpeakerLayoutIdx)
                << std::endl;
      break;
    case CMhasConfigPacket::SSpeakerConfig3d::ESpeakerLayoutType::CICPSPEAKERIDX:
      std::cout << "CICP Speaker Index" << std::endl;
      std::cout << prefix << "CICP speaker indices: ";
      for (const uint8_t speakerIdx : speakerConfig.cicpSpeakerIdx) {
        std::cout << std::to_string(speakerIdx) << ",";
      }
      std::cout << std::endl;
      break;
    case CMhasConfigPacket::SSpeakerConfig3d::ESpeakerLayoutType::FLEXIBLESPEAKERCONFIG:
      std::cout << "Flexible Speaker Layout" << std::endl;
      std::cout << prefix << "Number of speakers: " << speakerConfig.numSpeakers << std::endl;
      break;
    case CMhasConfigPacket::SSpeakerConfig3d::ESpeakerLayoutType::CONTRIBUTIONMODE:
      std::cout << "Contribution Mode" << std::endl;
      std::cout << prefix << "Number of speakers: " << speakerConfig.numSpeakers << std::endl;
      break;
    default:
      std::cout << "Invalid Speaker Config" << std::endl;
      break;
  }
}

static void printSignalGroup(const CMhasConfigPacket::SSignals3d::SSignalGroup& signalGroup,
                             const std::string& prefix = "") {
  std::cout << prefix << "Signal Group Index: " << static_cast<uint16_t>(signalGroup.idx)
            << std::endl;
  std::cout << prefix << "Signal Group Type: ";
  switch (signalGroup.signalGroupType) {
    case CMhasConfigPacket::SSignals3d::SSignalGroup::ESignalGroupType::CHANNELS:
      std::cout << "Channels" << std::endl;
      std::cout << prefix << "Number of audio channels: " << signalGroup.numSignals << std::endl;
      std::cout << prefix << "Audio Channel Layout:" << std::endl;

      printSpeakerConfig3d(*signalGroup.audioChannelLayout, prefix + "  ");
      break;
    case CMhasConfigPacket::SSignals3d::SSignalGroup::ESignalGroupType::HOA:
      std::cout << "HOA" << std::endl;
      std::cout << prefix << "Number of HOA transport channels: " << signalGroup.numSignals
                << std::endl;
      break;
    case CMhasConfigPacket::SSignals3d::SSignalGroup::ESignalGroupType::OBJECT:
      std::cout << "Objects" << std::endl;
      std::cout << prefix << "Number of Objects: " << signalGroup.numSignals << std::endl;
      break;
    case CMhasConfigPacket::SSignals3d::SSignalGroup::ESignalGroupType::INVALID:
    default:
      std::cout << "Invalid" << std::endl;
      break;
  }
}

int main(int argc, char* argv[]) {
  CMhasInfoWrapper mhasWrapper;

  if (argc != 2) {
    printUsage();
    return -1;
  }

  std::string inputFile(argv[1]);

  std::ifstream inStream(inputFile, std::ios_base::binary | std::ios_base::in);

  if (!inStream) {
    std::cout << "Error opening input file: " << inputFile << std::endl;
    return -1;
  }

  ilo::ByteBuffer buffer(8192);
  CMhasInfoWrapper::SMhasBufferInfo info;
  while (!inStream.eof()) {
    // Read from input file
    inStream.read(reinterpret_cast<char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size()));
    buffer.resize(static_cast<size_t>(inStream.gcount()));

    mhasWrapper.feed(buffer);
    if (!mhasWrapper.isMhasInfoAvailable()) {
      // If we cannot access the MHAS info yet continue to feed until we can.
      continue;
    }

    info = mhasWrapper.getMhasInfo();

    std::cout << "#########################" << std::endl << std::endl;
    std::cout << "Reference Layout: " << std::endl;
    printSpeakerConfig3d(*info.referenceLayout, "   ");
    std::cout << std::endl;

    std::cout << "Number of groups: " << info.groups.size() << std::endl;

    for (const auto& group : info.groups) {
      std::cout << std::endl;
      std::cout << "   Group Id: " << static_cast<uint16_t>(group.second.id) << std::endl;

      printItemDescriptions(group.second.descriptions);

      std::cout << "   Referenced Signal Groups:" << std::endl;

      std::map<std::shared_ptr<CMhasConfigPacket::SSignals3d::SSignalGroup>, std::vector<uint8_t>>
          signalMap;
      for (const auto& signal : group.second.signals) {
        signalMap[signal.signalGroup].push_back(signal.signalNumber);
      }

      std::string prefix = "     ";
      for (const auto& signalGroup : signalMap) {
        std::cout << std::endl;
        printSignalGroup(*signalGroup.first, prefix);
        std::cout << prefix << "Referenced Signals: ";
        if ((signalGroup.first->signalGroupType ==
                 CMhasConfigPacket::SSignals3d::SSignalGroup::ESignalGroupType::CHANNELS &&
             signalGroup.second.size() == signalGroup.first->numSignals) ||
            (signalGroup.first->signalGroupType ==
                 CMhasConfigPacket::SSignals3d::SSignalGroup::ESignalGroupType::HOA &&
             signalGroup.second.size() == signalGroup.first->numSignals) ||
            (signalGroup.first->signalGroupType ==
                 CMhasConfigPacket::SSignals3d::SSignalGroup::ESignalGroupType::OBJECT &&
             signalGroup.second.size() == signalGroup.first->numSignals)) {
          std::cout << "all";
        } else {
          for (uint8_t idx : signalGroup.second) {
            std::cout << static_cast<uint16_t>(idx) << ", ";
          }
        }
        std::cout << std::endl;
      }

      std::cout << std::endl;
    }

    std::cout << "Number of switch groups: " << info.switchGroups.size() << std::endl;

    for (const auto& switchGroup : info.switchGroups) {
      std::cout << std::endl;
      std::cout << "   Switch group id: " << static_cast<uint16_t>(switchGroup.second.id)
                << std::endl;

      std::cout << std::endl;
      printItemDescriptions(switchGroup.second.descriptions);

      std::cout << "   Referenced groups: ";
      for (uint8_t groupId : switchGroup.second.groupIds) {
        std::cout << static_cast<uint16_t>(groupId)
                  << (switchGroup.second.defaultGroupId == groupId ? "(default)" : "") << ",";
      }
      std::cout << std::endl;
    }

    std::cout << std::endl;

    std::cout << "Number of group presets: " << info.groupPresets.size() << std::endl;
    for (const auto& groupPreset : info.groupPresets) {
      std::cout << std::endl;
      std::cout << "   Group preset id: " << static_cast<uint16_t>(groupPreset.second.id)
                << std::endl;

      printItemDescriptions(groupPreset.second.descriptions);

      if (groupPreset.second.groupIds.size() == 1 &&
          groupPreset.second.groupIds[0].referenceId == 127 &&
          !groupPreset.second.groupIds[0].onOff) {
        std::cout << "   Special preset with full user interactivity." << std::endl;
        continue;
      }

      std::cout << "   Referenced groups: ";

      for (const auto& groupReference : groupPreset.second.groupIds) {
        std::cout << static_cast<uint16_t>(groupReference.referenceId) << " ";
        if (groupReference.groupType ==
            CMhasInfoWrapper::SGroupPreset::SGroupReference::EGroupType::Group) {
          std::cout << "(group id) ";
        } else {
          std::cout << "(switch group id) ";
        }

        if (groupReference.onOff) {
          std::cout << "(On)";
        } else {
          std::cout << "(Off)";
        }

        std::cout << ", ";
      }
      std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Number of Signal Groups: " << info.signalGroups.size() << std::endl;
    for (const auto& signalGroup : info.signalGroups) {
      std::cout << std::endl;
      printSignalGroup(*signalGroup, "   ");
    }

    std::cout << std::endl;
    std::cout << std::endl;
  }

  return 0;
}
