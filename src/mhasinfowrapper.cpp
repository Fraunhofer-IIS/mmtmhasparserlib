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
#include <exception>

// Internal includes
#include "mmtmhasparserlib/mhasinfowrapper.h"
#include "mmtmhasparserlib/mhaspacket.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"
#include "logging.h"

using namespace mmt::mhasparserlib;

void CMhasInfoWrapper::feed(const std::vector<uint8_t>& buffer) {
  m_mhasParser.feed(buffer);

  try {
    m_mhasParser.parsePackets();
  } catch (const std::exception& e) {
    m_mhasParser.reset();
    m_isMhasInfoAvailable = false;
    m_seekingAsi = false;

    m_mhasBufferInfo.wasResynced = true;

    ILO_LOG_ERROR("Error while parsing MHAS packets: %s\nResetting MHAS parser.\n", e.what());
  }

  handleParsedPackets();
}

bool CMhasInfoWrapper::isMhasInfoAvailable() const {
  return m_isMhasInfoAvailable;
}

CMhasInfoWrapper::SMhasBufferInfo CMhasInfoWrapper::getMhasInfo() {
  ILO_ASSERT(m_isMhasInfoAvailable, "No MHAS info available.");

  CMhasInfoWrapper::SMhasBufferInfo retInfo = m_mhasBufferInfo;
  m_mhasBufferInfo.wasResynced = false;

  return retInfo;
}

void CMhasInfoWrapper::extractAsiInfo(const CMhasAsiPacket& mhasAsiPacket) {
  using EDataType = CMhasAsiPacket::SAudioSceneDataElement::EDataType;
  auto audioScene = mhasAsiPacket.audioSceneInfo();

  m_seekingAsi = false;

  std::vector<uint8_t> groupPresetIds;

  initGroups(audioScene);
  initSwitchGroups(audioScene);
  initGroupPresets(audioScene, groupPresetIds);

  for (const auto& dataSet : audioScene.data.dataSets) {
    switch (dataSet.dataType) {
      case EDataType::ID_MAE_GROUP_PRESET_EXTENSION: {
        const auto& presetExt =
            dynamic_cast<CMhasAsiPacket::SAudioSceneGrpPresetEx&>(*dataSet.data);
        handleGroupPresetExtension(presetExt, groupPresetIds);
        break;
      }
      case EDataType::ID_MAE_SWITCHGROUP_DESCRIPTION:
      case EDataType::ID_MAE_GROUP_PRESET_DESCRIPTION:
      case EDataType::ID_MAE_GROUP_DESCRIPTION: {
        const auto& audioSceneDescription =
            dynamic_cast<CMhasAsiPacket::SAudioSceneDescription&>(*dataSet.data);
        handleGroupDescription(audioSceneDescription, dataSet.dataType);
        break;
      }
      default:
        break;
    }
  }
}

void CMhasInfoWrapper::initGroups(const CMhasAsiPacket::SAudioSceneInfo& audioScene) {
  m_mhasBufferInfo.groups.clear();
  for (const auto& group : audioScene.groups) {
    SGroup groupInfo;
    groupInfo.id = group.groupID;

    std::vector<uint8_t> metaDataElementIds;
    if (group.hasConjunctMembers) {
      for (uint8_t id = group.startID; id < group.startID + group.bsGroupNumMembers + 1; id++) {
        metaDataElementIds.push_back(id);
      }
    } else {
      metaDataElementIds = group.metaDataElementId;
    }

    for (uint8_t id : metaDataElementIds) {
      ILO_ASSERT(m_signals3d.signals.find(id) != m_signals3d.signals.end(),
                 "Metadata element id not found.");

      groupInfo.signals.push_back(m_signals3d.signals[id]);
    }

    m_mhasBufferInfo.groups[groupInfo.id] = groupInfo;
  }
}

void CMhasInfoWrapper::initSwitchGroups(const CMhasAsiPacket::SAudioSceneInfo& audioScene) {
  m_mhasBufferInfo.switchGroups.clear();
  for (const auto& switchGroup : audioScene.switchGroups) {
    SSwitchGroup switchGroupInfo;
    switchGroupInfo.id = switchGroup.switchGroupID;
    switchGroupInfo.defaultGroupId = switchGroup.switchGroupDefaultGroupId;

    for (uint8_t groupId : switchGroup.switchGroupMemberId) {
      ILO_ASSERT(m_mhasBufferInfo.groups.find(groupId) != m_mhasBufferInfo.groups.end(),
                 "No group with the given id was found.");
      switchGroupInfo.groupIds.push_back(groupId);
    }

    m_mhasBufferInfo.switchGroups[switchGroupInfo.id] = switchGroupInfo;
  }
}

void CMhasInfoWrapper::initGroupPresets(const CMhasAsiPacket::SAudioSceneInfo& audioScene,
                                        std::vector<uint8_t>& groupPresetIds) {
  m_mhasBufferInfo.groupPresets.clear();
  for (const auto& groupPreset : audioScene.groupPresets) {
    SGroupPreset groupPresetInfo;
    groupPresetInfo.id = groupPreset.presetID;

    for (const auto& condition : groupPreset.conditions) {
      SGroupPreset::SGroupReference ref;
      ref.referenceId = condition.groupID;
      ref.groupType = SGroupPreset::SGroupReference::EGroupType::Group;
      ref.onOff = condition.onOff;

      groupPresetInfo.groupIds.push_back(ref);
    }

    m_mhasBufferInfo.groupPresets[groupPresetInfo.id] = groupPresetInfo;
    groupPresetIds.push_back(groupPresetInfo.id);
  }
}

void CMhasInfoWrapper::handleGroupPresetExtension(
    const CMhasAsiPacket::SAudioSceneGrpPresetEx& presetExt,
    const std::vector<uint8_t>& groupPresetIds) {
  ILO_ASSERT(presetExt.groupPresets.size() == groupPresetIds.size(),
             "Group preset extension does not fit the list of group presets.");

  // Go over all group presets listed in the group preset extension (should be the same number as
  // before)
  for (size_t i = 0; i < presetExt.groupPresets.size(); i++) {
    if (presetExt.groupPresets[i].hasSwitchGrpConditions) {
      ILO_ASSERT(m_mhasBufferInfo.groupPresets[groupPresetIds[i]].groupIds.size() ==
                     presetExt.groupPresets[i].isSwitchGrpCondition.size(),
                 "The number of groups listed in the group preset extension does not fit the "
                 "number of groups listed in the group preset.");

      // Go over all groups listed in the group preset and check whether we need to set them to type
      // switch group
      for (size_t j = 0; j < presetExt.groupPresets[i].isSwitchGrpCondition.size(); j++) {
        uint8_t groupId = m_mhasBufferInfo.groupPresets[groupPresetIds[i]].groupIds[j].referenceId;
        if (presetExt.groupPresets[i].isSwitchGrpCondition[j]) {
          m_mhasBufferInfo.groupPresets[groupPresetIds[i]].groupIds[j].groupType =
              SGroupPreset::SGroupReference::EGroupType::SwitchGroup;

          ILO_ASSERT(m_mhasBufferInfo.switchGroups.count(groupId) == 1,
                     "No switch group with the given id (%d) was found.", groupId);
        } else {
          ILO_ASSERT(m_mhasBufferInfo.groups.count(groupId) == 1,
                     "No group with the given id (%d) was found.", groupId);
        }
      }
    }
  }
}

void CMhasInfoWrapper::handleGroupDescription(
    const CMhasAsiPacket::SAudioSceneDescription& audioSceneDescription,
    const CMhasAsiPacket::SAudioSceneDataElement::EDataType& dataType) {
  for (const auto& descBlock : audioSceneDescription.descriptionBlocks) {
    for (const auto& lang : descBlock.languages) {
      SItemDescription itemDesc;

      // Copy the language code
      for (int8_t i = 2; i >= 0; i--) {
        itemDesc.lang.at(static_cast<uint8_t>(2 - i)) =
            static_cast<char>((lang.bsDescLanguage >> 8 * i) & 0x00FF);
      }

      // Copy the description
      for (uint8_t c : lang.descData) {
        itemDesc.desc += static_cast<char>(c);
      }

      // Check where this description belongs
      switch (dataType) {
        case CMhasAsiPacket::SAudioSceneDataElement::EDataType::ID_MAE_GROUP_DESCRIPTION:
          ILO_ASSERT(m_mhasBufferInfo.groups.count(descBlock.descriptionGroupID) == 1,
                     "No group with the given id was found.");
          m_mhasBufferInfo.groups[descBlock.descriptionGroupID].descriptions.push_back(itemDesc);
          break;
        case CMhasAsiPacket::SAudioSceneDataElement::EDataType::ID_MAE_SWITCHGROUP_DESCRIPTION:
          ILO_ASSERT(m_mhasBufferInfo.switchGroups.count(descBlock.descriptionSwitchGroupID) == 1,
                     "No Switch group with the given id was found.");
          m_mhasBufferInfo.switchGroups[descBlock.descriptionSwitchGroupID].descriptions.push_back(
              itemDesc);
          break;
        case CMhasAsiPacket::SAudioSceneDataElement::EDataType::ID_MAE_GROUP_PRESET_DESCRIPTION:
          ILO_ASSERT(m_mhasBufferInfo.groupPresets.count(descBlock.descriptionGroupPresetID) == 1,
                     "No Group Preset with the given id was found.");
          m_mhasBufferInfo.groupPresets[descBlock.descriptionGroupPresetID].descriptions.push_back(
              itemDesc);
          break;
        default:
          // This cannot happen since we already make sure we are in one of the above states before
          // calling the function.
          ILO_FAIL("Invalid state reached.");
          break;
      }
    }
  }
}

void CMhasInfoWrapper::handleParsedPackets() {
  while (auto parsedMhasPackets = m_mhasParser.nextPacket()) {
    switch (static_cast<EMhasPacketType>(parsedMhasPackets->packetType())) {
      case EMhasPacketType::PACTYP_MPEGH3DACFG: {
        CMhasConfigPacket& mhasConfigPacket = dynamic_cast<CMhasConfigPacket&>(*parsedMhasPackets);
        CMhasConfigPacket::SConfig mhasConfigInfo = mhasConfigPacket.mhasConfigInfo();

        m_mhasBufferInfo.referenceLayout = mhasConfigInfo.referenceLayout;
        m_mhasBufferInfo.signalGroups = mhasConfigInfo.signals3d.signalGroups;
        m_signals3d = mhasConfigInfo.signals3d;

        m_seekingAsi = true;

        break;
      }
      case EMhasPacketType::PACTYP_AUDIOSCENEINFO: {
        CMhasAsiPacket& asiPacket = dynamic_cast<CMhasAsiPacket&>(*parsedMhasPackets);
        extractAsiInfo(asiPacket);

        m_isMhasInfoAvailable = true;
      } break;
      case EMhasPacketType::PACTYP_MPEGH3DAFRAME:
        if (m_seekingAsi) {
          // There was no ASI for the previous config. Forget all info we got so far.

          m_mhasBufferInfo.groups.clear();
          m_mhasBufferInfo.groupPresets.clear();
          m_mhasBufferInfo.switchGroups.clear();

          m_seekingAsi = false;
        }
        m_isMhasInfoAvailable = true;

        break;
      default:
        break;
    }
  }
}
