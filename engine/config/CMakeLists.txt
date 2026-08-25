# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# ConfigManagerUVE uses the root-owned pinned nlohmann::json dependency and
# confines it behind ConfigManagerUVE's PIMPL (config_manager_uve.cpp is the
# only file in the repo that includes <nlohmann/json.hpp>) — linked PRIVATE so
# it never leaks into consumers' include paths.

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
