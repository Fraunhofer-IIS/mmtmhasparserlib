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
#include <cmath>
#include <stdexcept>

// Internal includes
#include "logging.h"
#include "mmtmhasparserlib/mhasasipacket.h"

using namespace mmt::mhasparserlib;

CMhasAsiPacket::CMhasAsiPacket(ilo::ByteBuffer::const_iterator& begin,
                               ilo::ByteBuffer::const_iterator end)
    : CMhasPacket(begin, end) {
  ILO_ASSERT_WITH(EMhasPacketType(packetType()) == EMhasPacketType::PACTYP_AUDIOSCENEINFO,
                  std::invalid_argument, "Invalid packet type.");
  auto payloadBeginIterator = m_payload.cbegin();
  m_sceneInfo = SAudioSceneInfo();
  m_sceneInfo.parsePayload(payloadBeginIterator, m_payload.end());

  payloadBeginIterator = m_payload.end();
  ILO_ASSERT_WITH(payloadBeginIterator == m_payload.end(), std::invalid_argument,
                  "Payload was not completely parsed (contains data after ASI).");
}

CMhasAsiPacket::CMhasAsiPacket(uint64_t label, ilo::ByteBuffer::const_iterator payloadBegin,
                               ilo::ByteBuffer::const_iterator payloadEnd)
    : CMhasPacket(static_cast<uint32_t>(EMhasPacketType::PACTYP_AUDIOSCENEINFO)) {
  payload(payloadBegin, payloadEnd);
  packetLabel(label);
}

void CMhasAsiPacket::payload(ilo::ByteBuffer::const_iterator begin,
                             ilo::ByteBuffer::const_iterator end) {
  auto beginCopy = begin;
  m_sceneInfo = SAudioSceneInfo();
  m_sceneInfo.parsePayload(begin, end);
  ILO_ASSERT_WITH(begin == end, std::invalid_argument,
                  "Payload was not completely parsed (contains data after ASI).");

  CMhasPacket::payload(beginCopy, end);
}

std::string CMhasAsiPacket::packetName() const {
  return "ASI-Packet";
}

void CMhasAsiPacket::SAudioSceneGroup::parsePayload(ilo::CBitParser& bitparser) {
  groupID = bitparser.read<uint8_t>(7);
  allowOnOff = bitparser.read<uint8_t>(1) == 1;
  defaultOnOff = bitparser.read<uint8_t>(1) == 1;

  allowPositionInteractivity = bitparser.read<uint8_t>(1) == 1;
  if (allowPositionInteractivity) {
    interactivityMinAzOffset = bitparser.read<uint8_t>(7);
    interactivityMaxAzOffset = bitparser.read<uint8_t>(7);

    interactivityMinElOffset = bitparser.read<uint8_t>(5);
    interactivityMaxElOffset = bitparser.read<uint8_t>(5);

    interactivityMinDistFactor = bitparser.read<uint8_t>(4);
    interactivityMaxDistFactor = bitparser.read<uint8_t>(4);
  }

  allowGainInteractivity = bitparser.read<uint8_t>(1) == 1;
  if (allowGainInteractivity) {
    interactivityMinGain = bitparser.read<uint8_t>(6);
    interactivityMaxGain = bitparser.read<uint8_t>(5);
  }

  bsGroupNumMembers = bitparser.read<uint8_t>(7);
  hasConjunctMembers = bitparser.read<uint8_t>(1) == 1;

  if (hasConjunctMembers) {
    startID = bitparser.read<uint8_t>(7);
  } else {
    metaDataElementId.resize(bsGroupNumMembers + 1);
    std::generate(metaDataElementId.begin(), metaDataElementId.end(),
                  [&bitparser] { return bitparser.read<uint8_t>(7); });
  }
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMinAzimuthOffsetInDegrees() const {
  ILO_ASSERT(allowPositionInteractivity,
             "Cannot calculate interactivity limits without valid values");
  return -1.5f * static_cast<float>(interactivityMinAzOffset);
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMaxAzimuthOffsetInDegrees() const {
  ILO_ASSERT(allowPositionInteractivity,
             "Cannot calculate interactivity limits without valid values");
  return 1.5f * static_cast<float>(interactivityMaxAzOffset);
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMinElevationOffsetInDegrees() const {
  ILO_ASSERT(allowPositionInteractivity,
             "Cannot calculate interactivity limits without valid values");
  return -3.0f * static_cast<float>(interactivityMinElOffset);
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMaxElevationOffsetInDegrees() const {
  ILO_ASSERT(allowPositionInteractivity,
             "Cannot calculate interactivity limits without valid values");
  return 3.0f * static_cast<float>(interactivityMaxElOffset);
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMinDistanceFactor() const {
  ILO_ASSERT(allowPositionInteractivity,
             "Cannot calculate interactivity limits without valid values");
  return std::pow(2.0f, static_cast<float>(interactivityMinDistFactor) - 12.0f);
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMaxDistanceFactor() const {
  ILO_ASSERT(allowPositionInteractivity,
             "Cannot calculate interactivity limits without valid values");
  return std::pow(2.0f, static_cast<float>(interactivityMaxDistFactor) - 12.0f);
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMinGainInDecibels() const {
  ILO_ASSERT(allowGainInteractivity, "Cannot calculate interactivity limits without valid values");
  return static_cast<float>(interactivityMinGain) - 63.0f;
}

float CMhasAsiPacket::SAudioSceneGroup::interactivityMaxGainInDecibels() const {
  ILO_ASSERT(allowGainInteractivity, "Cannot calculate interactivity limits without valid values");
  return static_cast<float>(interactivityMaxGain);
}

void CMhasAsiPacket::SAudioSceneSwitchGroup::parsePayload(ilo::CBitParser& bitparser) {
  switchGroupID = bitparser.read<uint8_t>(5);
  switchGroupAllowOnOff = bitparser.read<uint8_t>(1) == 1;

  if (switchGroupAllowOnOff) {
    switchGroupDefaultOnOff = bitparser.read<uint8_t>(1) == 1;
  }

  uint8_t members = bitparser.read<uint8_t>(5);
  switchGroupMemberId.resize(members + 1);
  std::generate(switchGroupMemberId.begin(), switchGroupMemberId.end(),
                [&bitparser] { return bitparser.read<uint8_t>(7); });

  switchGroupDefaultGroupId = bitparser.read<uint8_t>(7);
}

float CMhasAsiPacket::SAudioSceneGroupPresets::SAudioScenePresetCondition::SCondition::
    azimuthOffsetInDegrees() const {
  ILO_ASSERT(positionFlag, "Cannot calculate position offsets without valid values");
  return 1.5f * (static_cast<float>(azOffset) - 127.0f);
}

float CMhasAsiPacket::SAudioSceneGroupPresets::SAudioScenePresetCondition::SCondition::
    elevationOffsetInDegrees() const {
  ILO_ASSERT(positionFlag, "Cannot calculate position offsets without valid values");
  return 3.0f * (static_cast<float>(elOffset) - 31.0f);
}

float CMhasAsiPacket::SAudioSceneGroupPresets::SAudioScenePresetCondition::SCondition::
    distanceChangeFactor() const {
  ILO_ASSERT(positionFlag, "Cannot calculate position offsets without valid values");
  return std::pow(2.0f, static_cast<float>(distFactor) - 12.0f);
}

void CMhasAsiPacket::SAudioSceneGroupPresets::parsePayload(ilo::CBitParser& bitparser) {
  presetID = bitparser.read<uint8_t>(5);
  kind = bitparser.read<uint8_t>(5);

  uint8_t numConditions = bitparser.read<uint8_t>(4);
  conditions.resize(numConditions + 1);
  std::generate(conditions.begin(), conditions.end(), [&bitparser] {
    CMhasAsiPacket::SAudioSceneGroupPresets::SAudioScenePresetCondition condition{};
    condition.groupID = bitparser.read<uint8_t>(7);
    condition.onOff = bitparser.read<uint8_t>(1) == 1;

    if (condition.onOff) {
      condition.condition = std::make_shared<
          CMhasAsiPacket::SAudioSceneGroupPresets::SAudioScenePresetCondition::SCondition>();
      condition.condition->disableGainInteractivity = bitparser.read<uint8_t>(1) == 1;
      condition.condition->gainFlag = bitparser.read<uint8_t>(1) == 1;
      if (condition.condition->gainFlag) {
        condition.condition->gain = bitparser.read<uint8_t>(8);
      }

      condition.condition->disablePositionInteractivity = bitparser.read<uint8_t>(1) == 1;
      condition.condition->positionFlag = bitparser.read<uint8_t>(1) == 1;
      if (condition.condition->positionFlag) {
        condition.condition->azOffset = bitparser.read<uint8_t>(8);
        condition.condition->elOffset = bitparser.read<uint8_t>(6);
        condition.condition->distFactor = bitparser.read<uint8_t>(4);
      }
    }
    return condition;
  });
}

void CMhasAsiPacket::SAudioSceneData::parsePayload(uint8_t numGroups, uint8_t numGroupPresets,
                                                   std::vector<uint8_t> grpPresetNumConditions,
                                                   ilo::CBitParser& bitparser) {
  uint8_t numDataSets = bitparser.read<uint8_t>(4);
  dataSets.resize(numDataSets);
  std::generate(dataSets.begin(), dataSets.end(), [&] {
    SAudioSceneDataSet dataSet;

    dataSet.dataType = static_cast<SAudioSceneDataElement::EDataType>(bitparser.read<uint8_t>(4));
    dataSet.dataLength = bitparser.read<uint16_t>(16);

    uint32_t numReadBitsOrig = bitparser.nofReadBits();

    switch (dataSet.dataType) {
      case SAudioSceneDataElement::EDataType::ID_MAE_GROUP_DESCRIPTION:
      case SAudioSceneDataElement::EDataType::ID_MAE_SWITCHGROUP_DESCRIPTION:
      case SAudioSceneDataElement::EDataType::ID_MAE_GROUP_PRESET_DESCRIPTION: {
        SAudioSceneDescription description;
        description.parsePayload(bitparser, dataSet.dataType);
        dataSet.data = std::make_shared<SAudioSceneDescription>(description);
        break;
      }
      case SAudioSceneDataElement::EDataType::ID_MAE_GROUP_CONTENT: {
        SAudioSceneContentDataBlock contentDataBlock;
        contentDataBlock.parsePayload(bitparser);
        dataSet.data = std::make_shared<SAudioSceneContentDataBlock>(contentDataBlock);
        break;
      }
      case SAudioSceneDataElement::EDataType::ID_MAE_GROUP_COMPOSITE: {
        SAudioSceneCompositePair compositePair;
        compositePair.parsePayload(bitparser);
        dataSet.data = std::make_shared<SAudioSceneCompositePair>(compositePair);
        break;
      }
      case SAudioSceneDataElement::EDataType::ID_MAE_SCREEN_SIZE: {
        SAudioSceneProdScreenSizeData prodScreenSizeData;
        prodScreenSizeData.parsePayload(bitparser);
        dataSet.data = std::make_shared<SAudioSceneProdScreenSizeData>(prodScreenSizeData);
        break;
      }
      case SAudioSceneDataElement::EDataType::ID_MAE_DRC_UI_INFO: {
        SAudioSceneDrcUiInfo drcUiInfo;
        drcUiInfo.parsePayload(bitparser, dataSet.dataLength);
        dataSet.data = std::make_shared<SAudioSceneDrcUiInfo>(drcUiInfo);
        break;
      }
      case SAudioSceneDataElement::EDataType::ID_MAE_SCREEN_SIZE_EXTENSION: {
        SAudioSceneProdScreenSizeDataEx prodScreenSizeDataEx;
        prodScreenSizeDataEx.parsePayload(bitparser);
        dataSet.data = std::make_shared<SAudioSceneProdScreenSizeDataEx>(prodScreenSizeDataEx);
        break;
      }
      case SAudioSceneDataElement::EDataType::ID_MAE_GROUP_PRESET_EXTENSION: {
        SAudioSceneGrpPresetEx grpPresetEx;
        for (uint8_t j = 0; j < numGroupPresets; ++j) {
          grpPresetEx.parsePayload(bitparser, grpPresetNumConditions[j]);
        }
        dataSet.data = std::make_shared<SAudioSceneGrpPresetEx>(grpPresetEx);
        break;
      }
      case SAudioSceneDataElement::EDataType::ID_MAE_LOUDNESS_COMPENSATION: {
        SAudioSceneLoudnessCompData loudnessCompData;
        loudnessCompData.parsePayload(bitparser, numGroups, numGroupPresets);
        dataSet.data = std::make_shared<SAudioSceneLoudnessCompData>(loudnessCompData);
        break;
      }
      default: {
        bitparser.seek(8 * dataSet.dataLength, ilo::EPosType::cur);

        break;
      }
    }

    auto bitsRead = bitparser.nofReadBits() - numReadBitsOrig;
    int32_t toRead = (dataSet.dataLength * 8) - static_cast<int32_t>(bitsRead);

    bitparser.seek(toRead, ilo::EPosType::cur);

    return dataSet;
  });
}

void CMhasAsiPacket::SAudioSceneCompositePair::parsePayload(ilo::CBitParser& bitparser) {
  uint8_t numCompositePairs = bitparser.read<uint8_t>(7);
  compositePairs.resize(numCompositePairs + 1);
  std::generate(compositePairs.begin(), compositePairs.end(), [&bitparser] {
    SCompositePair compositePair{};
    compositePair.compositeElementIDs[0] = bitparser.read<uint8_t>(7);
    compositePair.compositeElementIDs[1] = bitparser.read<uint8_t>(7);

    return compositePair;
  });
}

void CMhasAsiPacket::SAudioSceneContentDataBlock::parsePayload(ilo::CBitParser& bitparser) {
  uint8_t numContentDataBlocks = bitparser.read<uint8_t>(7);
  contentDataBlocks.resize(numContentDataBlocks + 1);
  std::generate(contentDataBlocks.begin(), contentDataBlocks.end(), [&bitparser] {
    SContentDataBlock contentDataBlock{};

    contentDataBlock.contentDataGroupID = bitparser.read<uint8_t>(7);
    contentDataBlock.contentKind = bitparser.read<uint8_t>(4);
    contentDataBlock.hasContentLanguage = bitparser.read<uint8_t>(1) == 1;

    if (contentDataBlock.hasContentLanguage) {
      contentDataBlock.contentLanguage = bitparser.read<uint32_t>(24);
    }

    return contentDataBlock;
  });
}

void CMhasAsiPacket::SAudioSceneDescription::parsePayload(ilo::CBitParser& bitparser,
                                                          EDataType type) {
  uint8_t numDescBlocks = bitparser.read<uint8_t>(7);
  descriptionBlocks.resize(numDescBlocks + 1);
  std::generate(descriptionBlocks.begin(), descriptionBlocks.end(), [&bitparser, type] {
    SDescriptionBlock descriptionBlock{};
    if (type == EDataType::ID_MAE_GROUP_DESCRIPTION) {
      descriptionBlock.descriptionGroupID = bitparser.read<uint8_t>(7);
    } else if (type == EDataType::ID_MAE_SWITCHGROUP_DESCRIPTION) {
      descriptionBlock.descriptionSwitchGroupID = bitparser.read<uint8_t>(5);
    } else if (type == EDataType::ID_MAE_GROUP_PRESET_DESCRIPTION) {
      descriptionBlock.descriptionGroupPresetID = bitparser.read<uint8_t>(5);
    }

    uint8_t numDescLanguages = bitparser.read<uint8_t>(4);
    descriptionBlock.languages.resize(numDescLanguages + 1);
    std::generate(descriptionBlock.languages.begin(), descriptionBlock.languages.end(),
                  [&bitparser] {
                    SDescriptionLanguages descLanguages{};
                    descLanguages.bsDescLanguage = bitparser.read<uint32_t>(24);
                    uint8_t descDataLength = bitparser.read<uint8_t>(8);
                    descLanguages.descData.resize(descDataLength + 1);
                    std::generate(descLanguages.descData.begin(), descLanguages.descData.end(),
                                  [&bitparser] { return bitparser.read<uint8_t>(8); });
                    return descLanguages;
                  });
    return descriptionBlock;
  });
}

float CMhasAsiPacket::SAudioSceneDrcUiInfo::STargetLoudnessConditions::
    targetLoudnessValueUpperInDecibels() const noexcept {
  return static_cast<float>(targetLoudnessValueUpper) - 63.0f;
}

void CMhasAsiPacket::SAudioSceneDrcUiInfo::parsePayload(ilo::CBitParser& bitparser,
                                                        uint16_t length) {
  version = bitparser.read<uint8_t>(2);

  if (version == 0) {
    uint8_t numTargetLoudnessConditions = bitparser.read<uint8_t>(3);
    targetLoudnessConditions.resize(numTargetLoudnessConditions);
    std::generate(targetLoudnessConditions.begin(), targetLoudnessConditions.end(), [&bitparser] {
      STargetLoudnessConditions targetLoudCond{};

      targetLoudCond.targetLoudnessValueUpper = bitparser.read<uint8_t>(6);
      targetLoudCond.drcSetEffectAvailable = bitparser.read<uint16_t>(16);

      return targetLoudCond;
    });
  } else {
    auto toSkip = length * 8 - 2;
    bitparser.seek(toSkip, ilo::EPosType::cur);
  }
}

float CMhasAsiPacket::SAudioSceneGrpPresetEx::SGrpPresetCondition::gainInDecibels() const noexcept {
  return 0.5f * (static_cast<float>(grpPresetGain) - 255.0f) + 32.0f;
}

float CMhasAsiPacket::SAudioSceneGrpPresetEx::SGrpPresetCondition::azimuthOffsetInDegrees()
    const noexcept {
  return 1.5f * (static_cast<float>(grpPresetAzOffset) - 127.0f);
}

float CMhasAsiPacket::SAudioSceneGrpPresetEx::SGrpPresetCondition::elevationOffsetInDegrees()
    const noexcept {
  return 3.0f * (static_cast<float>(grpPresetElOffset) - 31.0f);
}

float CMhasAsiPacket::SAudioSceneGrpPresetEx::SGrpPresetCondition::distanceChangeFactor()
    const noexcept {
  return std::pow(2.0f, static_cast<float>(grpPresetDistFactor) - 12.0f);
}

void CMhasAsiPacket::SAudioSceneGrpPresetEx::SDownmixIdGrpPresetEx::parseConditions(
    ilo::CBitParser& bitparser) {
  uint8_t numConditions = bitparser.read<uint8_t>(4);
  grpPresetConditions.resize(numConditions + 1);
  std::generate(grpPresetConditions.begin(), grpPresetConditions.end(), [&bitparser] {
    SGrpPresetCondition grpPresetCondition{};
    grpPresetCondition.isSwitchGrpCondtion = bitparser.read<uint8_t>(1) == 1;

    if (grpPresetCondition.isSwitchGrpCondtion) {
      grpPresetCondition.grpPresetSwitchGrpId = bitparser.read<uint8_t>(5);
    } else {
      grpPresetCondition.grpPresetGrpId = bitparser.read<uint8_t>(7);
    }

    grpPresetCondition.grpPresetConditionOnOff = bitparser.read<uint8_t>(1) == 1;

    if (grpPresetCondition.grpPresetConditionOnOff) {
      grpPresetCondition.grpPresetDisableGainInteractivity = bitparser.read<uint8_t>(1) == 1;
      ;
      grpPresetCondition.grpPresetGainFlag = bitparser.read<uint8_t>(1) == 1;

      if (grpPresetCondition.grpPresetGainFlag) {
        grpPresetCondition.grpPresetGain = bitparser.read<uint8_t>(8);
      }

      grpPresetCondition.grpPresetDisablePosInteractivity = bitparser.read<uint8_t>(1) == 1;
      grpPresetCondition.grpPresetPositionFlag = bitparser.read<uint8_t>(1) == 1;

      if (grpPresetCondition.grpPresetPositionFlag) {
        grpPresetCondition.grpPresetAzOffset = bitparser.read<uint8_t>(8);
        grpPresetCondition.grpPresetElOffset = bitparser.read<uint8_t>(6);
        grpPresetCondition.grpPresetDistFactor = bitparser.read<uint8_t>(4);
      }
    }
    return grpPresetCondition;
  });
}

void CMhasAsiPacket::SAudioSceneGrpPresetEx::SGroupPresets::parseDownmixIdGrpPresetEx(
    ilo::CBitParser& bitparser) {
  uint8_t numDownmixIdGrpPresetEx = bitparser.read<uint8_t>(5);
  downmixIdGrpPreset.reserve(numDownmixIdGrpPresetEx + 1);
  for (uint8_t j = 1; j < numDownmixIdGrpPresetEx + 1; ++j) {
    SDownmixIdGrpPresetEx downmixIdGrpPresetEx{};

    downmixIdGrpPresetEx.grpPresetDownmixId = bitparser.read<uint8_t>(7);
    downmixIdGrpPresetEx.parseConditions(bitparser);
    downmixIdGrpPreset.push_back(downmixIdGrpPresetEx);
  }
}

void CMhasAsiPacket::SAudioSceneGrpPresetEx::parsePayload(ilo::CBitParser& bitparser,
                                                          uint8_t grpPresetNumConditions) {
  SGroupPresets grpPresets;

  grpPresets.hasSwitchGrpConditions = bitparser.read<uint8_t>(1) == 1;

  if (grpPresets.hasSwitchGrpConditions) {
    grpPresets.isSwitchGrpCondition.resize(grpPresetNumConditions + 1);
    std::generate(grpPresets.isSwitchGrpCondition.begin(), grpPresets.isSwitchGrpCondition.end(),
                  [&bitparser] { return bitparser.read<uint8_t>(1) == 1; });
  }

  grpPresets.hasDownmixIdGrpPresetEx = bitparser.read<uint8_t>(1) == 1;

  if (grpPresets.hasDownmixIdGrpPresetEx) {
    grpPresets.parseDownmixIdGrpPresetEx(bitparser);
  }
  groupPresets.push_back(grpPresets);
}

float CMhasAsiPacket::SAudioSceneLoudnessCompData::SLcPresetParams::
    minPresetLoudnessCompensationInDecibels() const noexcept {
  return -3.0f * static_cast<float>(bsLcPresetMinGain);
}

float CMhasAsiPacket::SAudioSceneLoudnessCompData::SLcPresetParams::
    maxPresetLoudnessCompensationInDecibels() const noexcept {
  return 3.0f * static_cast<float>(bsLcPresetMaxGain);
}

void CMhasAsiPacket::SAudioSceneLoudnessCompData::parsePayload(ilo::CBitParser& bitparser,
                                                               uint8_t numGroups,
                                                               uint8_t numGroupPresets) {
  lcGroupLoudnessPresent = bitparser.read<uint8_t>(1) == 1;

  if (lcGroupLoudnessPresent) {
    bsLcGroupLoudness.resize(numGroups);
    std::generate(bsLcGroupLoudness.begin(), bsLcGroupLoudness.end(),
                  [&bitparser] { return bitparser.read<uint8_t>(8); });
  }

  lcDefaultParamsPresent = bitparser.read<uint8_t>(1) == 1;

  if (lcDefaultParamsPresent) {
    lcDefaultIncludeGroup.resize(numGroups);
    std::generate(lcDefaultIncludeGroup.begin(), lcDefaultIncludeGroup.end(),
                  [&bitparser] { return bitparser.read<uint8_t>(1) == 1; });

    lcDefaultMinMaxGainPresent = bitparser.read<uint8_t>(1) == 1;

    if (lcDefaultMinMaxGainPresent) {
      bsLcDefaultMinGain = bitparser.read<uint8_t>(4);
      bsLcDefaultMaxGain = bitparser.read<uint8_t>(4);
    }
  }

  lcPresetParams.resize(numGroupPresets);
  std::generate(lcPresetParams.begin(), lcPresetParams.end(), [&bitparser, numGroups] {
    SLcPresetParams presetParams{};

    presetParams.lcPresetParamsPresent = bitparser.read<uint8_t>(1) == 1;

    if (presetParams.lcPresetParamsPresent) {
      presetParams.lcPresetIncludeGroup.resize(numGroups);
      std::generate(presetParams.lcPresetIncludeGroup.begin(),
                    presetParams.lcPresetIncludeGroup.end(),
                    [&bitparser] { return bitparser.read<uint8_t>(1) == 1; });

      presetParams.lcPresetMinMaxGainPresent = bitparser.read<uint8_t>(1) == 1;

      if (presetParams.lcPresetMinMaxGainPresent) {
        presetParams.bsLcPresetMinGain = bitparser.read<uint8_t>(4);
        presetParams.bsLcPresetMaxGain = bitparser.read<uint8_t>(4);
      }
    }

    return presetParams;
  });
}

float CMhasAsiPacket::SAudioSceneLoudnessCompData::groupLoudnessCompensationInDecibels(
    uint8_t groupId) const {
  ILO_ASSERT_WITH(groupId < bsLcGroupLoudness.size(), std::invalid_argument,
                  "No loudness compensation value for group ID %d", groupId);
  return 0.25f * static_cast<float>(bsLcGroupLoudness[groupId]) - 57.75f;
}

float CMhasAsiPacket::SAudioSceneLoudnessCompData::minDefaultLoudnessCompensationInDecibels()
    const noexcept {
  return -3.0f * static_cast<float>(bsLcDefaultMinGain);
}

float CMhasAsiPacket::SAudioSceneLoudnessCompData::maxDefaultLoudnessCompensationInDecibels()
    const noexcept {
  return 3.0f * static_cast<float>(bsLcDefaultMaxGain);
}

void CMhasAsiPacket::SAudioSceneProdScreenSizeData::parsePayload(ilo::CBitParser& bitparser) {
  hasNonStandardScreenSize = bitparser.read<uint8_t>(1) == 1;

  if (hasNonStandardScreenSize) {
    bsScreenSizeAz = bitparser.read<uint16_t>(9);
    bsScreenSizeTopEl = bitparser.read<uint16_t>(9);
    bsScreenSizeBottomEl = bitparser.read<uint16_t>(9);
  }
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeData::nominalLeftScreenEdgeInDegrees()
    const noexcept {
  if (!hasNonStandardScreenSize) {
    return 29.0f;
  }
  auto tmp = 0.5f * static_cast<float>(bsScreenSizeAz);
  return std::min(std::max(tmp, 0.0f), 180.0f);
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeData::nominalRightScreenEdgeInDegrees()
    const noexcept {
  if (!hasNonStandardScreenSize) {
    return -29.0f;
  }
  auto tmp = -0.5f * static_cast<float>(bsScreenSizeAz);
  return std::min(std::max(tmp, -180.0f), 0.0f);
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeData::nominalTopScreenEdgeInDegrees()
    const noexcept {
  if (!hasNonStandardScreenSize) {
    return 17.5f;
  }
  auto tmp = 0.5f * (static_cast<float>(bsScreenSizeTopEl) - 255.0f);
  return std::min(std::max(tmp, -90.0f), 90.0f);
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeData::nominalBottomScreenEdgeInDegrees()
    const noexcept {
  if (!hasNonStandardScreenSize) {
    return -17.5f;
  }
  auto tmp = 0.5f * (static_cast<float>(bsScreenSizeBottomEl) - 255.0f);
  return std::min(std::max(tmp, -90.0f), 90.0f);
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeDataEx::SPresetProductionScreens::
    nominalLeftScreenEdgeInDegrees() const {
  ILO_ASSERT(hasNonStandardScreenSize, "Cannot calculate screen edges without valid data");
  if (isCenteredInAzimuth) {
    auto tmp = 0.5f * static_cast<float>(bsScreenSizeAz);
    return std::min(std::max(tmp, 0.0f), 180.0f);
  } else {
    auto tmp = 0.5f * (static_cast<float>(bsScreenSizeLeftAz) - 511.0f);
    return std::min(std::max(tmp, -180.0f), 180.0f);
  }
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeDataEx::SPresetProductionScreens::
    nominalRightScreenEdgeInDegrees() const {
  ILO_ASSERT(hasNonStandardScreenSize, "Cannot calculate screen edges without valid data");
  if (isCenteredInAzimuth) {
    auto tmp = -0.5f * static_cast<float>(bsScreenSizeAz);
    return std::min(std::max(tmp, -180.0f), 0.0f);
  } else {
    auto tmp = 0.5f * (static_cast<float>(bsScreenSizeRightAz) - 511.0f);
    return std::min(std::max(tmp, -180.0f), 180.0f);
  }
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeDataEx::SPresetProductionScreens::
    nominalTopScreenEdgeInDegrees() const {
  ILO_ASSERT(hasNonStandardScreenSize, "Cannot calculate screen edges without valid data");
  auto tmp = 0.5f * (static_cast<float>(bsScreenSizeTopEl) - 255.0f);
  return std::min(std::max(tmp, -90.0f), 90.0f);
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeDataEx::SPresetProductionScreens::
    nominalBottomScreenEdgeInDegrees() const {
  ILO_ASSERT(hasNonStandardScreenSize, "Cannot calculate screen edges without valid data");
  auto tmp = 0.5f * (static_cast<float>(bsScreenSizeBottomEl) - 255.0f);
  return std::min(std::max(tmp, -90.0f), 90.0f);
}

void CMhasAsiPacket::SAudioSceneProdScreenSizeDataEx::parsePayload(ilo::CBitParser& bitparser) {
  overwriteProdScreenSizeData = bitparser.read<uint8_t>(1) == 1;

  if (overwriteProdScreenSizeData) {
    bsScreenSizeLeftAz = bitparser.read<uint16_t>(10);
    bsScreenSizeRightAz = bitparser.read<uint16_t>(10);
  }

  uint8_t numPresetProductionScreens = bitparser.read<uint8_t>(5);
  presetProdScreens.resize(numPresetProductionScreens);
  std::generate(presetProdScreens.begin(), presetProdScreens.end(), [&bitparser] {
    SPresetProductionScreens presetProdScreen{};

    presetProdScreen.prodScreenGrpPresetId = bitparser.read<uint8_t>(5);
    presetProdScreen.hasNonStandardScreenSize = bitparser.read<uint8_t>(1) == 1;

    if (presetProdScreen.hasNonStandardScreenSize) {
      presetProdScreen.isCenteredInAzimuth = bitparser.read<uint8_t>(1) == 1;

      if (presetProdScreen.isCenteredInAzimuth) {
        presetProdScreen.bsScreenSizeAz = bitparser.read<uint16_t>(9);
      } else {
        presetProdScreen.bsScreenSizeLeftAz = bitparser.read<uint16_t>(10);
        presetProdScreen.bsScreenSizeRightAz = bitparser.read<uint16_t>(10);
      }

      presetProdScreen.bsScreenSizeTopEl = bitparser.read<uint16_t>(9);
      presetProdScreen.bsScreenSizeBottomEl = bitparser.read<uint16_t>(9);
    }

    return presetProdScreen;
  });
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeDataEx::nominalLeftScreenEdgeInDegrees() const {
  ILO_ASSERT(overwriteProdScreenSizeData, "Cannot calculate screen edges without valid data");
  auto tmp = 0.5f * (static_cast<float>(bsScreenSizeLeftAz) - 511.0f);
  return std::min(std::max(tmp, -180.0f), 180.0f);
}

float CMhasAsiPacket::SAudioSceneProdScreenSizeDataEx::nominalRightScreenEdgeInDegrees() const {
  ILO_ASSERT(overwriteProdScreenSizeData, "Cannot calculate screen edges without valid data");
  auto tmp = 0.5f * (static_cast<float>(bsScreenSizeRightAz) - 511.0f);
  return std::min(std::max(tmp, -180.0f), 180.0f);
}

void CMhasAsiPacket::SAudioSceneInfo::parsePayload(ilo::ByteBuffer::const_iterator& begin,
                                                   ilo::ByteBuffer::const_iterator end) {
  ILO_ASSERT_WITH(end > begin, std::invalid_argument,
                  "End iterator is smaller or equal end iterator.");

  ilo::CBitParser bitparser(begin, end);

  isMainStream = bitparser.read<uint8_t>(1) == 1;
  if (isMainStream) {
    audioSceneInfoIDPresent = bitparser.read<uint8_t>(1) == 1;
    if (audioSceneInfoIDPresent) {
      audioSceneInfoID = bitparser.read<uint8_t>(8);
    }

    // parse groups
    uint8_t numGroups = bitparser.read<uint8_t>(7);
    groups.resize(numGroups);
    std::generate(groups.begin(), groups.end(), [&bitparser] {
      SAudioSceneGroup obj{};
      obj.parsePayload(bitparser);
      return obj;
    });

    // parse switch groups
    uint8_t numSwitchGroups = bitparser.read<uint8_t>(5);
    switchGroups.resize(numSwitchGroups);
    std::generate(switchGroups.begin(), switchGroups.end(), [&bitparser] {
      SAudioSceneSwitchGroup obj{};
      obj.parsePayload(bitparser);
      return obj;
    });

    // parse presets
    uint8_t numGroupPresets = bitparser.read<uint8_t>(5);
    groupPresets.resize(numGroupPresets);
    std::generate(groupPresets.begin(), groupPresets.end(), [&bitparser] {
      SAudioSceneGroupPresets obj{};
      obj.parsePayload(bitparser);
      return obj;
    });

    // parse generic maeData
    std::vector<uint8_t> numConditions;
    numConditions.reserve(groupPresets.size());
    for (const auto& groupPreset : groupPresets) {
      numConditions.push_back(static_cast<uint8_t>(groupPreset.conditions.size() - 1));
    }

    data.parsePayload(numGroups, numGroupPresets, numConditions, bitparser);

    metaDataElementIDmaxAvail = bitparser.read<uint8_t>(7);
  } else {
    metaDataElementIDOffset = bitparser.read<uint8_t>(7);
    metaDataElementIDmaxAvail = bitparser.read<uint8_t>(7);
  }

  auto numReadBits = bitparser.nofReadBits();

  begin += (numReadBits % 8 == 0) ? numReadBits / 8 : (numReadBits / 8) + 1;
}
