# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# ConfigManagerUVE is the first module with a third-party data-format
# dependency: nlohmann::json, fetched here and confined behind
# ConfigManagerUVE's PIMPL (config_manager_uve.cpp is the only file in the
# repo that includes <nlohmann/json.hpp>) — linked PRIVATE so it never
# leaks into consumers' include paths.

include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
set(JSON_Examples OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(uve_config STATIC
    src/config_manager_uve.cpp
)
target_include_directories(uve_config PUBLIC include)
target_link_libraries(uve_config
    PUBLIC uve_platform uve_debug
    PRIVATE nlohmann_json::nlohmann_json
)
target_compile_features(uve_config PUBLIC cxx_std_20)
uve_set_warnings(uve_config)
