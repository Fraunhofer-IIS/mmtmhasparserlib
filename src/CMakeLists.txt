FetchContent_MakeAvailable(ilo mmtaudioparser)

configure_file (
    "${PROJECT_SOURCE_DIR}/src/mhasparserlib_config.h.in"
    "${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/version.h"
)

add_library(mmtmhasparserlib STATIC
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/version.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhasparser.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhaspacket.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhassyncpacket.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhascrc16packet.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhasmarkerpacket.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhasframepacket.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhasconfigpacket.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhastruncationpacket.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhasasipacket.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhashelpertools.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhasinfowrapper.h
  ${PROJECT_SOURCE_DIR}/include/mmtmhasparserlib/mhasutilities.h
  logging.h
  mhasparser.cpp
  mhaspacket.cpp
  mhassyncpacket.cpp
  mhascrc16packet.cpp
  mhasframepacket.cpp
  mhasconfigpacket.cpp
  mhastruncationpacket.cpp
  mhasmarkerpacket.cpp
  mhasasipacket.cpp
  mhashelpertools.cpp
  mhasinfowrapper.cpp
  mhasutilities.cpp
)
target_compile_features(mmtaudioparser PUBLIC cxx_std_11)
set_target_properties(mmtaudioparser PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(mmtmhasparserlib PUBLIC ilo mmtaudioparser)
target_include_directories(mmtmhasparserlib PUBLIC ${PROJECT_SOURCE_DIR}/include)
