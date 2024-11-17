cmake_minimum_required(VERSION 3.28.1)

set(CMAKE_GENERATOR_PLATFORM x64)

project(erdyes
  VERSION   "1.0.1"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include(FetchContent)

FetchContent_Declare(mini
  GIT_REPOSITORY        https://github.com/metayeti/mINI.git
  GIT_TAG               0.9.15
  CONFIGURE_COMMAND     ""
  BUILD_COMMAND         "")

set(SPDLOG_DISABLE_DEFAULT_LOGGER ON)
set(SPDLOG_USE_STD_FORMAT ON)
FetchContent_Declare(spdlog
  GIT_REPOSITORY        https://github.com/gabime/spdlog.git
  GIT_TAG               v1.13.0)

FetchContent_Declare(elden-x
  GIT_REPOSITORY        https://github.com/ThomasJClark/elden-x.git
  GIT_TAG               57eb2a928a3550e721fa2406fcbb40628a5f4583)

# # Set iterator debug level to 0 for ELDEN RING ABI compatibility
add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)

FetchContent_MakeAvailable(mini spdlog elden-x)

add_library(mini INTERFACE)
target_include_directories(mini INTERFACE ${mini_SOURCE_DIR}/src)

add_library(erdyes SHARED
  src/erdyes_colors.cpp  
  src/erdyes_config.cpp
  src/erdyes_messages.cpp
  src/erdyes_messages_by_lang.cpp
  src/erdyes_state.cpp  
  src/erdyes_talkscript.cpp
  src/dllmain.cpp)

set_target_properties(erdyes PROPERTIES OUTPUT_NAME "erdyes")

add_definitions(-DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

add_custom_command(TARGET erdyes POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:erdyes>
  ${CMAKE_SOURCE_DIR}/LICENSE.txt
  ${CMAKE_SOURCE_DIR}/erdyes.ini
  COMMAND_EXPAND_LISTS)

target_link_libraries(erdyes mini spdlog elden-x)
