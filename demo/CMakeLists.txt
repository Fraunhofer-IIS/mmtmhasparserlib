FetchContent_MakeAvailable(mmtisobmff)

add_executable(mhasparser mhasparser.cpp)
add_executable(mhmparser mhmparser.cpp common.cpp common.h)
add_executable(mhasprint mhasprint.cpp)
add_executable(configparser configparser.cpp)

target_link_libraries(mhasparser mmtmhasparserlib)
target_link_libraries(mhmparser mmtmhasparserlib mmtisobmff)
target_link_libraries(mhasprint mmtmhasparserlib)
target_link_libraries(configparser mmtmhasparserlib)

target_include_directories(mhmparser PRIVATE ../src)
target_include_directories(mhasprint PRIVATE ../src)
target_include_directories(configparser PRIVATE ../src)
