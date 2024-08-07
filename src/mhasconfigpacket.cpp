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
#include <map>
#include <stdexcept>
#include <vector>

// External includes
#include "mmtaudioparser/mpeghparser.h"

// Internal includes
#include "logging.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"

using namespace mmt::mhasparserlib;

// Table 10 of ISO/IEC 23008-3 2nd ed
static const std::map<int32_t, double> RESAMPLING_RATIOS{
    {96000, 1.0}, {88200, 1.0}, {64000, 1.5}, {58800, 1.5}, {48000, 1.0}, {44100, 1.0},
    {32000, 1.5}, {29400, 1.5}, {24000, 2.0}, {22050, 2.0}, {16000, 3.0}, {14700, 3.0},
};

CMhasConfigPacket::CMhasConfigPacket(ilo::ByteBuffer::const_iterator& begin,
                                     ilo::ByteBuffer::const_iterator end)
    : CMhasPacket(begin, end) {
  ILO_ASSERT_WITH(EMhasPacketType(packetType()) == EMhasPacketType::PACTYP_MPEGH3DACFG,
                  std::invalid_argument, "Invalid packet type.");
  ilo::ByteBuffer::const_iterator buffBeg = m_payload.begin();
  ilo::ByteBuffer::const_iterator buffEnd = m_payload.end();
  m_config.parsePayload(buffBeg, buffEnd);
}

CMhasConfigPacket::CMhasConfigPacket(uint64_t label, ilo::ByteBuffer::const_iterator payloadStart,
                                     ilo::ByteBuffer::const_iterator payloadEnd)
    : CMhasPacket(static_cast<uint32_t>(EMhasPacketType::PACTYP_MPEGH3DACFG)) {
  payload(payloadStart, payloadEnd);
  packetLabel(label);
}

void CMhasConfigPacket::SConfig::parsePayload(ilo::ByteBuffer::const_iterator begin,
                                              ilo::ByteBuffer::const_iterator end) {
  ILO_ASSERT(begin < end, "Invalid payload provided.");

  ilo::ByteBuffer payload(begin, end);
  mmt::audioparser::CMpeghParser parser;

  parser.addConfig(payload);

  auto configInfo = parser.getConfigInfo();

  profileLevelIndication = configInfo.profileLevelIndicator;
  samplingFrequency = static_cast<int32_t>(configInfo.samplingFrequency);
  coreSbrFrameLengthIndex = configInfo.coreSbrFrameLengthIndex;

  ILO_ASSERT(profileLevelIndication >= 0x0B && profileLevelIndication <= 0x14,
             "Unsupported profile level indication");

  // the following scope is only allowed to be executed for BaseLine and LC Profile (use if, if
  // ASSERT above is removed)
  {
    ILO_ASSERT(coreSbrFrameLengthIndex == 1, "Wrong coreSbrFrameLengthIndex found.");

    if (profileLevelIndication == 0x0F || profileLevelIndication == 0x14) {
      if (samplingFrequency == 48000 || samplingFrequency == 32000) {
        ILO_LOG_WARNING(
            "Ambiguous resampling ratio found. Assuming output sampling frequency of 48000.");
      }
      if (samplingFrequency == 44100 || samplingFrequency == 29400) {
        ILO_LOG_WARNING(
            "Ambiguous resampling ratio found. Assuming output sampling frequency of 44100.");
      }
    } else {
      ILO_ASSERT(samplingFrequency <= 48000, "Invalid sampling frequency found.");
    }

    uint32_t coreFrameLength = 1024;

    auto ratioIt = RESAMPLING_RATIOS.find(samplingFrequency);
    ILO_ASSERT(ratioIt != RESAMPLING_RATIOS.end(), "Unknown sampling frequency found.");
    outputSamplingFrequency = static_cast<int32_t>(samplingFrequency * ratioIt->second);
    outputFramesize = static_cast<int32_t>(coreFrameLength * ratioIt->second);
  }

  referenceLayout = std::make_shared<SSpeakerConfig3d>();

  switch (configInfo.referenceLayout.speakerLayoutType) {
    case 0:
      referenceLayout->speakerLayoutType =
          SSpeakerConfig3d::ESpeakerLayoutType::CICPSPEAKERLAYOUTIDX;
      break;
    case 1:
      referenceLayout->speakerLayoutType = SSpeakerConfig3d::ESpeakerLayoutType::CICPSPEAKERIDX;
      break;
    case 2:
      referenceLayout->speakerLayoutType =
          SSpeakerConfig3d::ESpeakerLayoutType::FLEXIBLESPEAKERCONFIG;
      break;
    case 3:
      referenceLayout->speakerLayoutType = SSpeakerConfig3d::ESpeakerLayoutType::CONTRIBUTIONMODE;
      break;
    default:
      referenceLayout->speakerLayoutType = SSpeakerConfig3d::ESpeakerLayoutType::INVALID;
      ILO_ASSERT(false, "Invalid speaker layout type found.");
      break;
  }

  referenceLayout->cicpSpeakerLayoutIdx = configInfo.referenceLayout.CICPIdx;
  referenceLayout->cicpSpeakerIdx = configInfo.referenceLayout.CICPSpeakerIdx;
  referenceLayout->numSpeakers = configInfo.referenceLayout.numSpeakers;

  for (size_t grp = 0; grp < configInfo.signalGroups.size(); grp++) {
    std::shared_ptr<SSignals3d::SSignalGroup> signalGroup =
        std::make_shared<SSignals3d::SSignalGroup>();
    signalGroup->idx = static_cast<uint8_t>(grp);
    signalGroup->numSignals = configInfo.signalGroups[grp].numSignals;

    switch (configInfo.signalGroups[grp].signalGroupType) {
      case 0:
        signalGroup->signalGroupType = SSignals3d::SSignalGroup::ESignalGroupType::CHANNELS;

        signalGroup->audioChannelLayout = std::make_shared<SSpeakerConfig3d>();

        switch (configInfo.signalGroups[grp].audioChannelLayout.speakerLayoutType) {
          case 0:
            signalGroup->audioChannelLayout->speakerLayoutType =
                SSpeakerConfig3d::ESpeakerLayoutType::CICPSPEAKERLAYOUTIDX;
            break;
          case 1:
            signalGroup->audioChannelLayout->speakerLayoutType =
                SSpeakerConfig3d::ESpeakerLayoutType::CICPSPEAKERIDX;
            break;
          case 2:
            signalGroup->audioChannelLayout->speakerLayoutType =
                SSpeakerConfig3d::ESpeakerLayoutType::FLEXIBLESPEAKERCONFIG;
            break;
          case 3:
            signalGroup->audioChannelLayout->speakerLayoutType =
                SSpeakerConfig3d::ESpeakerLayoutType::CONTRIBUTIONMODE;
            break;
          default:
            signalGroup->audioChannelLayout->speakerLayoutType =
                SSpeakerConfig3d::ESpeakerLayoutType::INVALID;
            ILO_ASSERT(false, "Invalid speaker layout type found.");
            break;
        }

        signalGroup->audioChannelLayout->numSpeakers =
            configInfo.signalGroups[grp].audioChannelLayout.numSpeakers;
        signalGroup->audioChannelLayout->cicpSpeakerLayoutIdx =
            configInfo.signalGroups[grp].audioChannelLayout.CICPIdx;
        signalGroup->audioChannelLayout->cicpSpeakerIdx =
            configInfo.signalGroups[grp].audioChannelLayout.CICPSpeakerIdx;
        break;
      case 1:
        signalGroup->signalGroupType = SSignals3d::SSignalGroup::ESignalGroupType::OBJECT;
        break;
      case 2:
        ILO_ASSERT(false, "SAOC is currently not supported");
        break;
      case 3:
        signalGroup->signalGroupType = SSignals3d::SSignalGroup::ESignalGroupType::HOA;
        break;
      default:
        signalGroup->signalGroupType = SSignals3d::SSignalGroup::ESignalGroupType::INVALID;
        ILO_ASSERT(false, "Invalid signal group type found.");
        break;
    }

    signalGroup->metaDataElementIds = configInfo.signalGroups[grp].metaDataElementIds;

    for (size_t i = 0; i < signalGroup->metaDataElementIds.size(); i++) {
      signals3d.signals[signalGroup->metaDataElementIds[i]].signalGroup = signalGroup;
      signals3d.signals[signalGroup->metaDataElementIds[i]].signalNumber = static_cast<uint8_t>(i);
    }

    signals3d.signalGroups.push_back(signalGroup);

    signals3d.numAudioChannels = configInfo.numAudioChannels;
    signals3d.numAudioObjects = configInfo.numAudioObjects;
    signals3d.numHoaTransportChannel = configInfo.numHOATransportChannels;
  }

  audioPreRollPresent = configInfo.audioPreRollPresent;
  compatibleProfileLevels = configInfo.compatibleProfileLevels;
}

void CMhasConfigPacket::SConfig::applyAsi(const CMhasAsiPacket::SAudioSceneInfo& audioSceneInfo) {
  if (!audioSceneInfo.isMainStream) {
    signals3d.applyAsi(audioSceneInfo);
  }
}

bool CMhasConfigPacket::SSpeakerConfig3d::operator==(const SSpeakerConfig3d& compare) const {
  if (compare.speakerLayoutType != speakerLayoutType) {
    return false;
  }
  switch (speakerLayoutType) {
    case ESpeakerLayoutType::INVALID:
      return true;
    case ESpeakerLayoutType::CICPSPEAKERLAYOUTIDX:
      return compare.cicpSpeakerLayoutIdx == cicpSpeakerLayoutIdx;
    case ESpeakerLayoutType::CICPSPEAKERIDX:
      return compare.numSpeakers == numSpeakers && compare.cicpSpeakerIdx == cicpSpeakerIdx;
    default:
      return compare.numSpeakers == numSpeakers;
  }
}

bool CMhasConfigPacket::SSpeakerConfig3d::operator!=(const SSpeakerConfig3d& compare) const {
  return !(*this == compare);
}

void CMhasConfigPacket::SSignals3d::applyAsi(
    const CMhasAsiPacket::SAudioSceneInfo& audioSceneInfo) {
  if (!asiApplied) {
    appliedMetaDataElementIdOffset =
        static_cast<uint8_t>(audioSceneInfo.metaDataElementIDOffset + 1);

    std::map<uint8_t, CMhasConfigPacket::SSignals3d::SSignal> newSignals;
    for (auto& signal : signals) {
      newSignals[static_cast<uint8_t>(signal.first + appliedMetaDataElementIdOffset)] =
          signal.second;
    }
    signals = newSignals;

    for (auto& signalGroup : signalGroups) {
      std::vector<uint8_t> newMetaDataElementIds;
      for (uint8_t id : signalGroup->metaDataElementIds) {
        newMetaDataElementIds.push_back(static_cast<uint8_t>(appliedMetaDataElementIdOffset + id));
      }
      signalGroup->metaDataElementIds = newMetaDataElementIds;
    }
    asiApplied = true;
  }
}

void CMhasConfigPacket::payload(ilo::ByteBuffer::const_iterator begin,
                                ilo::ByteBuffer::const_iterator end) {
  m_config.parsePayload(begin, end);
  CMhasPacket::payload(begin, end);
}

CMhasConfigPacket::SConfig CMhasConfigPacket::mhasConfigInfo() const {
  return m_config;
}

bool CMhasConfigPacket::isLcProfile() const {
  return m_config.profileLevelIndication >= 0x0B && m_config.profileLevelIndication <= 0x0F;
}

std::string CMhasConfigPacket::packetName() const {
  return "Config-Packet";
}
