/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2024 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>

// External includes
#include "ilo/common_types.h"
#include "mmtisobmff/writer/output.h"

// Internal includes
#include "common.h"
#include "logging.h"
#include "mmtmhasparserlib/pesparser.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"
#include "mmtmhasparserlib/mhashelpertools.h"

using namespace mmt::mhasparserlib;

static void printHelp() {
  std::cout << "Usage: <pestomhm> [-t <timestamp>] -o <output MP4 file> [<input PES file>]"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Parameter:" << std::endl;
  std::cout << "  -h, --help           Show this help and exit." << std::endl;
  std::cout << "  -t <timestamp>       The creation timestamp to set for the MP4 file in seconds "
               "since midnight 1904-01-01.\n"
            << "                       This option should be only used for testing purposes, i.e. "
               "to create reproducible files."
            << std::endl;
  std::cout << "  -o <output MP4 file> The output file to write the generated MP4 contents to."
            << std::endl;
  std::cout << "  <input PES file>     The input file to read the MPEG PES stream from.\n"
            << "                       If not specified, the PES packets are read from the "
               "standard input."
            << std::endl;
}

int main(int argc, char** argv) {
  if (argc > 6 || argc < 3) {
    // "-o <output file>" is mandatory
    printHelp();
    return EXIT_FAILURE;
  }

  // Redirect mmtisobmff logs to syslog
  LOG_REDIRECT_TO_SYSTEM_LOG();

  mmt::isobmff::SMovieConfig movieConfig{};
  // Default values for the compatible brands and the major brand
  movieConfig.compatibleBrands = {ilo::toFcc("mp42")};
  movieConfig.majorBrand = ilo::toFcc("mp42");

  std::string inputFileName = "stdin";
  std::ifstream inputFile{};
  std::istream* input = &std::cin;
  std::string outputFileName{};
  std::unique_ptr<mmt::isobmff::CIsobmffFileWriter> output{};

  for (int i = 1; i < argc; ++i) {
    if (std::string{"-h"} == argv[i] || std::string{"--help"} == argv[i]) {
      printHelp();
      return EXIT_SUCCESS;
    } else if (std::string{"-t"} == argv[i]) {
      if ((i + 1) >= argc) {
        printHelp();
        return EXIT_FAILURE;
      }
      movieConfig.currentTimeInUtc = std::stoull(argv[i + 1]);
      ++i;
    } else if (std::string{"-o"} == argv[i]) {
      if ((i + 1) >= argc) {
        printHelp();
        return EXIT_FAILURE;
      }
      outputFileName = argv[i + 1];
      mmt::isobmff::CIsobmffFileWriter::SOutputConfig outputConfig{};
      outputConfig.outputUri = outputFileName;
      output = ilo::make_unique<mmt::isobmff::CIsobmffFileWriter>(outputConfig, movieConfig);
      ++i;
    } else if (i == argc - 1) {
      inputFileName = argv[i];
      inputFile.open(inputFileName, std::ios::binary);
      input = &inputFile;
    } else {
      printHelp();
      return EXIT_FAILURE;
    }
  }

  if (outputFileName.empty()) {
    std::cout << "No output MP4 file specified!\n" << std::endl;
    printHelp();
    return EXIT_FAILURE;
  } else if (!input || !*input) {
    std::cout << "Error opening input file: " << inputFileName << std::endl;
    return EXIT_FAILURE;
  } else if (!output) {
    std::cout << "Error opening output file: " << outputFileName << std::endl;
    return EXIT_FAILURE;
  }

  std::unique_ptr<CMhmOutput> mhmOutput{};
  CPesParser pesParser{};
  ilo::ByteBuffer chunk(4096);
  do {
    input->read(reinterpret_cast<char*>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
    pesParser.feed(chunk.data(), static_cast<std::size_t>(input->gcount()));

    if (!mhmOutput) {
      pesParser.parseAccessUnits();
      if (auto au = pesParser.nextAccessUnit()) {
        // read configuration from first MHAS config packet
        auto configPacketIter =
            tools::findPacketWithType(au.packets, EMhasPacketType::PACTYP_MPEGH3DACFG);
        if (configPacketIter == au.packets.end()) {
          throw std::runtime_error{
              "There has to be a MHAS config packet before the first MHAS frame packet."};
        }
        const auto* configPacket = dynamic_cast<const CMhasConfigPacket*>(configPacketIter->get());
        ILO_ASSERT(configPacket, "Failed to read MHAS config packet");

        mhmOutput = ilo::make_unique<CMhmOutput>(
            std::move(output), au.sampleRate, true /* allow multi-stream */,
            configPacket->mhasConfigInfo().compatibleProfileLevels);
        mhmOutput->writeSample(std::move(au.packets), au.duration, au.isIpf);
      } else {
        // need more input data to produce first acces unit
        continue;
      }
    }

    pesParser.parseAccessUnits();
    while (auto au = pesParser.nextAccessUnit()) {
      mhmOutput->writeSample(std::move(au.packets), au.duration, au.isIpf);
    }
  } while (input && *input);
  return EXIT_SUCCESS;
}
