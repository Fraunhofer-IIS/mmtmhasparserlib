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

/*!
 * @file mhasasipacket.h
 *
 * @brief MHAS ASI packet definitions
 */
#pragma once

// System includes
#include <array>
#include <memory>
#include <string>
#include <vector>

// External includes
#include "ilo/bitparser.h"
#include "ilo/common_types.h"

// Internal includes
#include "version.h"
#include "mhaspacket.h"

namespace mmt {
namespace mhasparserlib {
/*!
 * @brief Definition of an MHAS Audio Scene Information (ASI) packet.
 */
class CMhasAsiPacket final : public CMhasPacket {
 public:
  //! Representation of a single group preset in the mae_GroupPresetDefinition() structure according
  //! to ISO/IEC 23008-3 subclause 15.2.
  struct SAudioSceneGroupPresets {
    //! Single group condition in a group preset
    struct SAudioScenePresetCondition {
      //! Group preset condition information
      struct SCondition {
        /*!
         * @brief Flag indicating whether the gain interactivity of the referenced metadata element
         * group will be disabled.
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        bool disableGainInteractivity = false;
        /*!
         * @brief Flag indicating whether this condition defines an initial gain value (@ref gain).
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        bool gainFlag = false;

        /*!
         * @brief The initial gain of the referenced metadata element group.
         *
         * The group preset gain in decibels (dB) is calculated from this value as follows:
         *   gain in dB = 0.5 * (@ref gain - 255) + 32
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        uint8_t gain = 0;

        /*!
         * @brief Flag indicating whether the position interactivity of the referenced metadata
         * element group will be disabled.
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        bool disablePositionInteractivity = false;

        /*!
         * @brief Flag indicating whether this condition defines additional azimuth and elevation
         * offsets.
         *
         * If this flag is set to false, the values of @ref azOffset, @ref elOffset and @ref
         * distFactor are not meaningful.
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        bool positionFlag = false;

        /*!
         * @brief The additional azimuth offset applied to the referenced metadata element group.
         *
         * The condition's azimuth offset in degrees is calculated from this value as follows:
         *   azimuth offset in degrees = 1.5 * (@ref azOffset - 127)
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        uint8_t azOffset = 0;

        /*!
         * @brief The additional elevation offset applied to the referenced metadata element group.
         *
         * The condition's elevation offset in degrees is calculated from this value as follows:
         *   elevation offset in degrees = 3 * (@ref
         * SAudioSceneGrpPresetEx::SGrpPresetCondition::grpPresetElOffset - 31)
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        uint8_t elOffset = 0;

        /*!
         * @brief The additional distance change factor applied to the referenced metadata element
         * group.
         *
         * The distance change factor is calculated from this value as follows:
         *  distance change factor = 2^(@ref
         * SAudioSceneGrpPresetEx::SGrpPresetCondition::grpPresetDistFactor - 12)
         *
         * @see ISO/IEC 23008-3 subclause 15.3
         */
        uint8_t distFactor = 0;

        //! Returns the value of @ref azOffset in degrees
        float azimuthOffsetInDegrees() const;

        //! Returns the value of @ref elOffset in degrees
        float elevationOffsetInDegrees() const;

        //! Returns the calculated value of @ref distFactor
        float distanceChangeFactor() const;
      };

      /*!
       * @brief The ID of the metadata element group referenced by this condition.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t groupID = 0;

      /*!
       * @brief Flag indicating whether the referenced metadata element group has to be turned on to
       * match this condition.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool onOff = false;

      //! The actual condition information, only available if @ref onOff is set to true (on).
      std::shared_ptr<SCondition> condition = nullptr;
    };

    /*!
     * @brief Unique ID for this group preset .
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t presetID = 0;

    /*!
     * @brief The kind of content of this group preset, according to ISO/IEC 23008-3 subclause 15.3.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t kind = 0;

    /*!
     * @brief The group conditions associated with this group preset.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    std::vector<SAudioScenePresetCondition> conditions;

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser);
  };

  //! Representation of a single switch group in the mae_SwitchGroupDefinition() structure according
  //! to ISO/IEC 23008-3 subclause 15.2.
  struct SAudioSceneSwitchGroup {
    /*!
     * @brief Unique ID for this switch group of metadata elements groups.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t switchGroupID = 0;

    /*!
     * @brief Flag indicating whether this switch group is allowed to be completely disabled by the
     * user.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool switchGroupAllowOnOff = false;

    /*!
     * @brief Flag indicating whether this switch group is enabled or disabled by default.
     *
     * For enabled switch groups, the default member is selected initially.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool switchGroupDefaultOnOff = false;

    /*!
     * @brief The ID of the group which is selected as default member of this switch group.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t switchGroupDefaultGroupId = 0;

    /*!
     * @brief The IDs of the metadata element groups that are contained in this switch group.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    std::vector<uint8_t> switchGroupMemberId;

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser);
  };

  //! Representation of a single group in the mae_GroupDefinition() structure according to ISO/IEC
  //! 23008-3 subclause 15.2.
  struct SAudioSceneGroup {
    /*!
     * @brief Unique ID for this group of metadata elements.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t groupID = 0;

    /*!
     * @brief Flag indicating whether this metadata element group is allowed to be enabled/disabled
     * by the user.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool allowOnOff = false;

    /*!
     * @brief Flag indicating whether this metadata element group is enabled or disabled by default.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool defaultOnOff = false;

    /*!
     * @brief Flag indicating whether the position of this group's elements is allowed to be changed
     * by the user.
     *
     * The position (azimuth, elevation, distance) interactivity fields in this object only have
     * usable values if this flag is set.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool allowPositionInteractivity = false;

    /*!
     * @brief The minimum azimuth offset for members of this metadata element group that can be
     * selected by the user.
     *
     * The effective minimum azimuth offset in degrees is calculated by this value via:
     *   minimum azimuth offset in degrees = -1.5 * @ref interactivityMinAzOffset
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMinAzOffset = 0;

    /*!
     * @brief The maximum azimuth offset for members of this metadata element group that can be
     * selected by the user.
     *
     * The effective maximum azimuth offset in degrees is calculated by this value via:
     *   maximum azimuth offset in degrees = 1.5 * @ref interactivityMaxAzOffset
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMaxAzOffset = 0;

    /*!
     * @brief The minimum elevation offset for members of this metadata element group that can be
     * selected by the user.
     *
     * The effective minimum elevation offset in degrees is calculated by this value via:
     *   minimum elevation offset in degrees = -3 * @ref interactivityMinElOffset
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMinElOffset = 0;

    /*!
     * @brief The maximum elevation offset for members of this metadata element group that can be
     * selected by the user.
     *
     * The effective maximum elevation offset in degrees is calculated by this value via:
     *   maximum elevation offset in degrees = 3 * @ref interactivityMaxElOffset
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMaxElOffset = 0;

    /*!
     * @brief The minimum distance factor for members of this metadata element group that can be
     * selected by the user.
     *
     * The effective minimum distance factor is calculated by this value via:
     *   minimum distance factor = 2^(@ref interactivityMinDistFactor - 12)
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMinDistFactor = 0;

    /*!
     * @brief The maximum distance factor for members of this metadata element group that can be
     * selected by the user.
     *
     * The effective maximum distance factor is calculated by this value via:
     *   maximum distance factor = 2^(@ref interactivityMaxDistFactor - 12)
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMaxDistFactor = 0;

    /*!
     * @brief Flag indicating whether the gain of this group's elements is allowed to be changed by
     * the user.
     *
     * The gain interactivity fields in this object only have usable values if this flag is set.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool allowGainInteractivity = false;

    /*!
     * @brief The minimum gain for members of this metadata element group that can be selected by
     * the user.
     *
     * The effective minimum gain in decibels is calculated by this value via:
     *   minimum gain in decibels = @ref interactivityMinGain - 63
     *
     * If this field is set to zero, the effective minimum gain is negative Infinite.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMinGain = 0;

    /*!
     * @brief The maximum gain for members of this metadata element group that can be selected by
     * the user.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t interactivityMaxGain = 0;

    /*!
     * @brief The number of members in this metadata element group.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t bsGroupNumMembers = 0;

    /*!
     * @brief Flag indicating whether the members in this metadata element group are coded
     * consecutively in the bitstream.
     *
     * If this flag is set, the @ref startID defines the ID of the first member element. Otherwise,
     * the IDs of all member elements are defined by @ref metaDataElementId.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool hasConjunctMembers = false;

    /*!
     * @brief For consecutive members, defines the ID of the first element in this group.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t startID = 0;

    /*!
     * @brief Contains the IDs of all members of this metadata element group for non-consecutive
     * member IDs.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    std::vector<uint8_t> metaDataElementId;

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser);

    //! Returns the value of @ref interactivityMinAzOffset in degrees
    float interactivityMinAzimuthOffsetInDegrees() const;

    //! Returns the value of @ref interactivityMaxAzOffset in degrees
    float interactivityMaxAzimuthOffsetInDegrees() const;

    //! Returns the value of @ref interactivityMinElOffset in degrees
    float interactivityMinElevationOffsetInDegrees() const;

    //! Returns the value of @ref interactivityMaxElOffset in degrees
    float interactivityMaxElevationOffsetInDegrees() const;

    //! Returns the calculated value of @ref interactivityMinDistFactor
    float interactivityMinDistanceFactor() const;

    //! Returns the calculated value of @ref interactivityMaxDistFactor
    float interactivityMaxDistanceFactor() const;

    //! Returns the value of @ref interactivityMinGain in decibels (dB)
    float interactivityMinGainInDecibels() const;

    //! Returns the value of @ref interactivityMaxGain in decibels (dB)
    float interactivityMaxGainInDecibels() const;
  };

  //! Base structure for all supported audio scene data elements
  struct SAudioSceneDataElement {
    //! The supported data types of audio scene data elements, as defined in ISO/IEC 23008-3
    //! subclause 15.3.
    enum class EDataType : uint8_t {
      ID_MAE_GROUP_DESCRIPTION = 0,
      ID_MAE_SWITCHGROUP_DESCRIPTION = 1,
      ID_MAE_GROUP_CONTENT = 2,
      ID_MAE_GROUP_COMPOSITE = 3,
      ID_MAE_SCREEN_SIZE = 4,
      ID_MAE_GROUP_PRESET_DESCRIPTION = 5,
      ID_MAE_DRC_UI_INFO = 6,
      ID_MAE_SCREEN_SIZE_EXTENSION = 7,
      ID_MAE_GROUP_PRESET_EXTENSION = 8,
      ID_MAE_LOUDNESS_COMPENSATION = 9
    };

    virtual ~SAudioSceneDataElement() noexcept = default;
  };

  //! Representation of the mae_CompositePair() structure according to ISO/IEC 23008-3
  //! subclause 15.2.
  struct SAudioSceneCompositePair final : SAudioSceneDataElement {
    //! A single composite pair entry
    struct SCompositePair {
      /*!
       * @brief Unique ID for a metadata element
       *
       * This field can take values between 0 and 127, resulting in a maximum of 128 metadata
       * elements. The value of position 0 denotes an independent object and the value of position 1
       * a dependent object.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      std::array<uint8_t, 2> compositeElementIDs;
    };

    //! The list of contained composite pairs
    std::vector<SCompositePair> compositePairs;

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser);
  };

  //! Representation of the mae_ContentData() structure according to ISO/IEC 23008-3 subclause 15.2.
  struct SAudioSceneContentDataBlock final : SAudioSceneDataElement {
    //! A single audio scene content data block
    struct SContentDataBlock {
      /*!
       * @brief The group ID of the group to which the ContentData block applies
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t contentDataGroupID = 0;

      /*!
       * @brief The kind of content of a metadata element group.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t contentKind = 0;

      /*!
       * @brief Whether the actual metadata element group has a language assigned to its content.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool hasContentLanguage = false;

      /*!
       * @brief 24-bit field identifying the language of a metadata element group.
       *
       * This field contains a 3-character code as specified by either ISO 639-2/B or ISO 639-2/T.
       * Each character is coded into 8 bits according to ISO/IEC 8859-1 and inserted in order into
       * the 24-bit field. EXAMPLE: French has 3-character code “fre”, which is coded as: “0110 0110
       * 0111 0010 0110 0101”.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint32_t contentLanguage = 0;
    };

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser);

    //! The contained content data blocks
    std::vector<SContentDataBlock> contentDataBlocks;
  };

  //! Representation of the mae_Description() structure according to ISO/IEC 23008-3 subclause 15.2.
  struct SAudioSceneDescription final : SAudioSceneDataElement {
    //! Structure for language-specific (localized) description data
    struct SDescriptionLanguages {
      /*!
       * @brief 24-bit field identifying the language of the description text of a metadata element
       * group.
       *
       * This field contains a 3-character code as specified by either ISO 639-2/B or ISO 639-2/T.
       * Each character is coded into 8 bits according to ISO/IEC 8859-1 and inserted in order into
       * the 24-bit field. EXAMPLE: French has 3-character code “fre”, which is coded as: “0110 0110
       * 0111 0010 0110 0101”.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint32_t bsDescLanguage = 0u;

      /*!
       * @brief UTF-8 string description of a metadata element group or a switch group.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      std::vector<uint8_t> descData;
    };

    //! Representation of a single description entry
    struct SDescriptionBlock {
      /*!
       * @brief The group ID of the group to which the description block applies.
       *
       * @note This field is only set for \ref
       * SAudioSceneDataElement::EDataType::ID_MAE_GROUP_DESCRIPTION descriptions.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t descriptionGroupID = 0u;

      /*!
       * @brief The switch group ID of the switch group to which the description block applies.
       *
       * @note This field is only set for \ref
       * SAudioSceneDataElement::EDataType::ID_MAE_SWITCHGROUP_DESCRIPTION descriptions.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t descriptionSwitchGroupID = 0u;

      /*!
       * @brief The group preset ID of the group preset to which the description block applies.
       *
       * @note This field is only set for \ref
       * SAudioSceneDataElement::EDataType::ID_MAE_GROUP_PRESET_DESCRIPTION descriptions.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t descriptionGroupPresetID = 0u;

      //! The language-specific entries for this description
      std::vector<SDescriptionLanguages> languages;
    };

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser, EDataType type);

    //! The single description blocks
    std::vector<SDescriptionBlock> descriptionBlocks;
  };

  //! Representation of the mae_DrcUserInterfaceInfo() structure according to ISO/IEC 23008-3
  //! subclause 15.2.
  struct SAudioSceneDrcUiInfo final : SAudioSceneDataElement {
    //! A single target loudness condition entry
    struct STargetLoudnessConditions {
      /*!
       * @brief Upper limit of the target loudness range as offset from -63 dB.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t targetLoudnessValueUpper = 0;

      /*!
       * @brief Available DRC set effects in the audio bitstream for this target loudness range.
       *
       * Each available DRC set effect type is represented by one bit in this field according to
       * ISO/IEC 23003-4.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint16_t drcSetEffectAvailable = 0;

      //! Returns the value of @ref targetLoudnessValueUpper in decibels (dB)
      float targetLoudnessValueUpperInDecibels() const noexcept;
    };

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser, uint16_t length);

    //! Version of this description structure, shall be zero.
    uint8_t version;
    //! The contained target loudness conditions, for description structures of verions zero.
    std::vector<STargetLoudnessConditions> targetLoudnessConditions;
  };

  //! Representation of the mae_GroupPresetDefinitionExtension() structure according to ISO/IEC
  //! 23008-3 subclause 15.2.
  struct SAudioSceneGrpPresetEx final : SAudioSceneDataElement {
    //! A single group preset condition entry
    struct SGrpPresetCondition {
      /*!
       * @brief The switch group ID associated with this condition.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t grpPresetSwitchGrpId = 0;

      /*!
       * @brief The group ID associated with this condition.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t grpPresetGrpId = 0;

      /*!
       * @brief The initial gain of the referenced metadata element (group or switch group members).
       *
       * The group preset gain in decibels (dB) is calculated from this value as follows:
       *   gain in dB = 0.5 * (@ref grpPresetGain - 255) + 32
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t grpPresetGain = 0;

      /*!
       * @brief The additional azimuth offset applied to the referenced metadata element (group or
       * switch group members).
       *
       * The group preset azimuth offset in degrees is calculated from this value as follows:
       *   azimuth offset in degrees = 1.5 * (@ref grpPresetAzOffset - 127)
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t grpPresetAzOffset = 0;

      /*!
       * @brief The additional elevation offset applied to the referenced metadata element (group or
       * switch group members).
       *
       * The group preset elevation offset in degrees is calculated from this value as follows:
       *   elevation offset in degrees = 3 * (@ref grpPresetElOffset - 31)
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t grpPresetElOffset = 0;

      /*!
       * @brief The additional distance change factor applied to the referenced metadata element
       * (group or switch group members).
       *
       * The distance change factor is calculated from this value as follows:
       *  distance change factor = 2^(@ref grpPresetDistFactor - 12)
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t grpPresetDistFactor = 0;

      /*!
       * @brief Flag indicating whether the referenced metadata element (group or switch group
       * members) has to be turned on to match this condition.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool grpPresetConditionOnOff = false;

      /*!
       * @brief Flag indicating whether the gain interactivity of the referenced metadata element
       * (group or switch group members) will be disabled.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool grpPresetDisableGainInteractivity = false;

      /*!
       * @brief Flag indicating whether this extension defines an initial gain value (@ref
       * grpPresetGain).
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool grpPresetGainFlag = false;

      /*!
       * @brief Flag indicating whether this is a condition on a referenced switch group (true) or
       * group (false).
       *
       * Depending on the value of this flag, the @ref grpPresetSwitchGrpId and @ref grpPresetGrpId
       * fields determine the referenced metadata elements.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool isSwitchGrpCondtion = false;

      /*!
       * @brief Flag indicating whether the position interactivity of the referenced metadata
       * element (group or switch group members) will be disabled.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool grpPresetDisablePosInteractivity = false;

      /*!
       * @brief Flag indicating whether this extension defines additional azimuth and elevation
       * offsets (@ref grpPresetAzOffset and @ref grpPresetElOffset).
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool grpPresetPositionFlag = false;

      //! Returns the value of @ref grpPresetGain in decibels (dB)
      float gainInDecibels() const noexcept;

      //! Returns the value of @ref grpPresetAzOffset in degrees
      float azimuthOffsetInDegrees() const noexcept;

      //! Returns the value of @ref grpPresetElOffset in degrees
      float elevationOffsetInDegrees() const noexcept;

      //! Returns the calculated value of @ref grpPresetDistFactor
      float distanceChangeFactor() const noexcept;
    };

    /*!
     * @brief A single downmix group preset extension.
     *
     * This extension overwrites the preset conditions and other preset characteristics for a
     * specific @ref grpPresetDownmixId.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    struct SDownmixIdGrpPresetEx {
      /*!
       * @brief The downmixId for which this extension applies.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint8_t grpPresetDownmixId = 0;

      //! The group preset conditions to apply for the associated downmix.
      std::vector<SGrpPresetCondition> grpPresetConditions;

      /*!
       * @brief Parses the associated audio scene structure from the given bit parser and fills this
       * object's fields accordingly.
       */
      void parseConditions(ilo::CBitParser& bitparser);
    };

    //! Representation of a single group preset entry
    struct SGroupPresets {
      /*!
       * @brief Flag indicating whether this group preset has switch group conditions.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool hasSwitchGrpConditions = false;

      /*!
       * @brief Flag indicating whether this group preset has layout-dependent extensions.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool hasDownmixIdGrpPresetEx = false;

      /*!
       * @brief Set of flags defining whether the i-th condition is a switch group condition.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      std::vector<bool> isSwitchGrpCondition;

      //! The downmix group preset extensions
      std::vector<SDownmixIdGrpPresetEx> downmixIdGrpPreset;

      /*!
       * @brief Parses the associated audio scene structure from the given bit parser and fills this
       * object's fields accordingly.
       */
      void parseDownmixIdGrpPresetEx(ilo::CBitParser& bitparser);
    };

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser, uint8_t grpPresetNumConditions);

    //! The single group preset entries
    std::vector<SGroupPresets> groupPresets;
  };

  //! Representation of the mae_LoudnessCompensationData() structure according to ISO/IEC 23008-3
  //! subclause 15.2.
  struct SAudioSceneLoudnessCompData final : SAudioSceneDataElement {
    /*!
     * @brief Container for preset-specific loudness compensation parameters.
     */
    struct SLcPresetParams {
      /*!
       * @brief Flag indicating whether loudness compensation parameters for the associated preset
       * are present.
       *
       * This flag needs to be set for the @ref lcPresetIncludeGroup vector to contain valid data.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool lcPresetParamsPresent = false;

      /*!
       * @brief Flag indicating whether the min/max values for the loudness compensation gain of the
       * associated preset are present.
       *
       * This flag needs to be set for the @ref bsLcPresetMinGain and @ref bsLcPresetMaxGain fields
       * to contain valid data.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool lcPresetMinMaxGainPresent = false;

      /*!
       * @brief The minimum value for the loudness compensation gain of the associated preset.
       *
       * The minimum loudness compensation value in decibel (dB) is calculated from this value as
       * follows: loudness compensation min gain in dB = -3 * @ref bsLcPresetMinGain
       */
      uint8_t bsLcPresetMinGain = 0;

      /*!
       * @brief The maximum value for the loudness compensation gain of the associated preset.
       *
       * The maximum loudness compensation value in decibel (dB) is calculated from this value as
       * follows: loudness compensation max gain in dB = 3 * @ref bsLcPresetMaxGain
       */
      uint8_t bsLcPresetMaxGain = 0;

      /*!
       * @brief The list of flags signaling whether the metadata element group with the
       * corresponding index (group ID) is incorporated in the computation of the loudness
       * compensation gain of the associated preset.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      std::vector<bool> lcPresetIncludeGroup;

      //! Returns the value of @ref bsLcPresetMinGain in decibels (dB)
      float minPresetLoudnessCompensationInDecibels() const noexcept;

      //! Returns the value of @ref bsLcPresetMaxGain in decibels (dB)
      float maxPresetLoudnessCompensationInDecibels() const noexcept;
    };

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser, uint8_t numGroups, uint8_t numGroupPresets);

    /*!
     * @brief Flag indicating whether group loudness values for loudness compensation are present.
     *
     * This flag needs to be set for the @ref bsLcGroupLoudness vector to contain valid data.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool lcGroupLoudnessPresent = false;

    /*!
     * @brief Flag indicating whether loudness compensation parameters for the default scene are
     * present.
     *
     * This flag needs to be set for the @ref lcDefaultIncludeGroup vector to contain valid data.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool lcDefaultParamsPresent = false;

    /*!
     * @brief Flag indicating whether the min/max values for the loudness compensation gain of the
     * default scene are present.
     *
     * This flag needs to be set for the @ref bsLcDefaultMinGain and @ref bsLcDefaultMaxGain fields
     * to contain valid data.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool lcDefaultMinMaxGainPresent = false;

    /*!
     * @brief The minimum value for the loudness compensation gain of the default scene.
     *
     * The minimum loudness compensation value in decibel (dB) is calculated from this value as
     * follows: loudness compensation default min gain in dB = -3 * @ref bsLcDefaultMinGain
     */
    uint8_t bsLcDefaultMinGain = 0;

    /*!
     * @brief The maximum value for the loudness compensation gain of the default scene.
     *
     * The maximum loudness compensation value in decibel (dB) is calculated from this value as
     * follows: loudness compensation default max gain in dB = 3 * @ref bsLcDefaultMaxGain
     */
    uint8_t bsLcDefaultMaxGain = 0;

    /*!
     * @brief The loudness compensation values for the metadata element group with the corresponding
     * index (group ID).
     *
     * The effective loudness compensation value is calculated from these values as follows:
     *   loudness Compensation group loudness in dB = 0.25 * @ref bsLcGroupLoudness - 57.75
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    std::vector<uint8_t> bsLcGroupLoudness;

    /*!
     * @brief The list of flags signaling whether the metadata element group with the corresponding
     * index (group ID) is incorporated in the computation of the loudness compensation gain of the
     * default scene.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    std::vector<bool> lcDefaultIncludeGroup;

    /*!
     * @brief Additional preset-specific loudness compensation parameters.
     *
     * The list contains one entry per preset and is indexed by the preset ID, i.e. the i-th
     * parameters correspond to the preset ID i.
     */
    std::vector<SLcPresetParams> lcPresetParams;

    //! Returns the value of the indexed @ref bsLcGroupLoudness in decibels (dB)
    float groupLoudnessCompensationInDecibels(uint8_t groupId) const;

    //! Returns the value of @ref bsLcDefaultMinGain in decibels (dB)
    float minDefaultLoudnessCompensationInDecibels() const noexcept;

    //! Returns the value of @ref bsLcDefaultMaxGain in decibels (dB)
    float maxDefaultLoudnessCompensationInDecibels() const noexcept;
  };

  //! Representation of the mae_ProductionScreenSizeData() structure according to ISO/IEC 23008-3
  //! subclause 15.2.
  struct SAudioSceneProdScreenSizeData final : SAudioSceneDataElement {
    /*!
     * @brief Flag indicating whether the audio scene has a non-standard size.
     *
     * If this flag is set to false, the other fields in this structure have no valid values.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool hasNonStandardScreenSize = false;

    /*!
     * @brief The non-standard screen size azimuth of the left and right screen edges.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint16_t bsScreenSizeAz = 0;

    /*!
     * @brief The non-standard screen size elevation of the top screen edge.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint16_t bsScreenSizeTopEl = 0;

    /*!
     * @brief The non-standard screen size elevation of the bottom screen edge.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint16_t bsScreenSizeBottomEl = 0;

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser);

    //! Returns the calculated left screen edge in degrees
    float nominalLeftScreenEdgeInDegrees() const noexcept;

    //! Returns the calculated right screen edge in degrees
    float nominalRightScreenEdgeInDegrees() const noexcept;

    //! Returns the calculated top screen edge in degrees
    float nominalTopScreenEdgeInDegrees() const noexcept;

    //! Returns the calculated bottom screen edge in degrees
    float nominalBottomScreenEdgeInDegrees() const noexcept;
  };

  //! Representation of the mae_ProductionScreenSizeDataExtension() structure according to ISO/IEC
  //! 23008-3 subclause 15.2.
  struct SAudioSceneProdScreenSizeDataEx final : SAudioSceneDataElement {
    //! Preset-specific production screen dimensions
    struct SPresetProductionScreens {
      /*!
       * @brief Flag indicating whether the audio scene has a non-standard size for the associated
       * preset.
       *
       * If this flag is set to false, the other fields (except the @ref prodScreenGrpPresetId) in
       * this structure have no valid values.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool hasNonStandardScreenSize = false;

      /*!
       * @brief Flag indicating whether the production screen for the associated preset is centered
       * in azimuth.
       *
       * If true, the absolute values of the azimuth angles of the left and right screen edge are
       * identical.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      bool isCenteredInAzimuth = false;

      //! The group preset ID this production screen size applies to.
      uint8_t prodScreenGrpPresetId = 0;

      /*!
       * @brief The azimuth corresponding to the left and right screen edges.
       *
       * @note This field is only valid if @ref isCenteredInAzimuth is set.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint16_t bsScreenSizeAz = 0;

      /*!
       * @brief The azimuth corresponding to the left screen edge.
       *
       * @note This field is only valid if @ref isCenteredInAzimuth is set to false.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint16_t bsScreenSizeLeftAz = 0;

      /*!
       * @brief The azimuth corresponding to the right screen edge.
       *
       * @note This field is only valid if @ref isCenteredInAzimuth is set to false.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint16_t bsScreenSizeRightAz = 0;

      /*!
       * @brief The elevation corresponding to the top screen edge.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint16_t bsScreenSizeTopEl = 0;

      /*!
       * @brief The elevation corresponding to the bottom screen edge.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint16_t bsScreenSizeBottomEl = 0;

      //! Returns the calculated left screen edge in degrees
      float nominalLeftScreenEdgeInDegrees() const;

      //! Returns the calculated right screen edge in degrees
      float nominalRightScreenEdgeInDegrees() const;

      //! Returns the calculated top screen edge in degrees
      float nominalTopScreenEdgeInDegrees() const;

      //! Returns the calculated bottom screen edge in degrees
      float nominalBottomScreenEdgeInDegrees() const;
    };

    /*!
     * @brief Flag indicating whether this bitstream contains azimuth for a non-centered default
     * production screen.
     *
     * If this flag is set to false, the @ref bsScreenSizeLeftAz and @ref bsScreenSizeRightAz fields
     * have no valid values.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool overwriteProdScreenSizeData = false;

    /*!
     * @brief The azimuth value of the left screen edge for non-centered production screens.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint16_t bsScreenSizeLeftAz = 0;

    /*!
     * @brief The azimuth value of the right screen edge for non-centered production screens.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint16_t bsScreenSizeRightAz = 0;

    //! Additional preset-specific production screen dimensions
    std::vector<SPresetProductionScreens> presetProdScreens;

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(ilo::CBitParser& bitparser);

    //! Returns the calculated left screen edge in degrees
    float nominalLeftScreenEdgeInDegrees() const;

    //! Returns the calculated right screen edge in degrees
    float nominalRightScreenEdgeInDegrees() const;
  };

  //! Container for additional audio scene data entries
  struct SAudioSceneData {
    //! A single additional audio scene data set
    struct SAudioSceneDataSet {
      /*!
       * @brief The type of description that follows in the bitstream.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      SAudioSceneDataElement::EDataType dataType;

      /*!
       * @brief The length in bytes of the data element that follows in the bitstream.
       *
       * @see ISO/IEC 23008-3 subclause 15.3
       */
      uint16_t dataLength = 0;

      //! The actual audio scene data element.
      std::shared_ptr<SAudioSceneDataElement> data;
    };

    //! The data sets contained in the audio scene
    std::vector<SAudioSceneDataSet> dataSets;

    /*!
     * @brief Parses the associated audio scene structure from the given bit parser and fills this
     * object's fields accordingly.
     */
    void parsePayload(uint8_t numGroups, uint8_t numGroupPresets,
                      std::vector<uint8_t> grpPresetNumConditions, ilo::CBitParser& bitparser);
  };

  //! Representation of the mae_AudioSceneInfo() structure according to ISO/IEC 23008-3
  //! subclause 15.2.
  struct SAudioSceneInfo {
    /*!
     * @brief Flag indicating whether this MPEG-H stream is the main stream.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool isMainStream = false;

    /*!
     * @brief Flag indicating whether an ID for the current audio scene is present in the bitstream.
     *
     * @ref audioSceneInfoID
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    bool audioSceneInfoIDPresent = false;

    /*!
     * @brief The ID of the current audio scene.
     *
     * An audio scene ID value of 0 means that the ID shall not be evaluated.
     * If the @ref audioSceneInfoIDPresent is false, this ID defaults to zero.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t audioSceneInfoID = 0u;

    /*!
     * @brief The offset for the first metadata element of the current MPEG-H data stream.
     *
     * For main streams, the offset is always zero.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t metaDataElementIDOffset = 0u;

    /*!
     * @brief The maximum available meta data element ID in the stream.
     *
     * @see ISO/IEC 23008-3 subclause 15.3
     */
    uint8_t metaDataElementIDmaxAvail = 0u;

    //! The groups in the overall audio scene.
    std::vector<SAudioSceneGroup> groups;
    //! The switch groups in the overall scene.
    std::vector<SAudioSceneSwitchGroup> switchGroups;
    //! The group presets in the audio scene
    std::vector<SAudioSceneGroupPresets> groupPresets;
    //! Additional audio scene data
    SAudioSceneData data;

    /*!
     * @brief Parses the given byte range and sets the object's fields accordingly.
     *
     * The given byte range must contain exactly a single MHAS ASI packet.
     *
     * The begin iterator is incremented by the number of bytes read to parse this structure.
     */
    void parsePayload(ilo::ByteBuffer::const_iterator& begin, ilo::ByteBuffer::const_iterator end);
  };

  /*!
   * @brief Initialize the ASI packet by reading the given byte range.
   *
   * The given byte range must contain exactly a single MHAS ASI packet.
   *
   * The begin iterator is incremented by the number of bytes read to parse this MHAS packet.
   */
  CMhasAsiPacket(ilo::ByteBuffer::const_iterator& begin, ilo::ByteBuffer::const_iterator end);

  /*!
   * @brief Initialize the MHAS packet by reading the given byte range and overwrite the @ref
   * packetLabel.
   *
   * The given byte range must contain exactly a single MHAS ASI packet.
   */
  CMhasAsiPacket(uint64_t label, ilo::ByteBuffer::const_iterator payloadBegin,
                 ilo::ByteBuffer::const_iterator payloadEnd);

  /*!
   * @brief Sets the payload buffer to the given byte range.
   *
   * The given byte range must contain exactly a single MHAS ASI packet.
   *
   * The given range is parsed and the packet representation of this object is updated with the
   * contents of the buffer.
   */
  void payload(ilo::ByteBuffer::const_iterator begin, ilo::ByteBuffer::const_iterator end) override;

  //! Returns the parsed audio scene information structure
  SAudioSceneInfo audioSceneInfo() const { return m_sceneInfo; }

 private:
  std::string packetName() const override;
  SAudioSceneInfo m_sceneInfo;
};
}  // namespace mhasparserlib
}  // namespace mmt
