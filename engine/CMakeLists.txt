# Copyright (c) 2026 UniVex Studios. All Rights Reserved.


# platform, debug, utilities, events, memory, threading, math moved to
# Engine/Runtime/{Platform,Core/Logging,Core/Utilities,Events,Core/Memory,
# Core/Threading,Core/Math} and added from the root CMakeLists.txt, ahead of
# add_subdirectory(engine), since modules below still link against their
# uve_* targets by name.
add_subdirectory(input)
add_subdirectory(commandline)
add_subdirectory(config)
add_subdirectory(asset)
add_subdirectory(scene)
add_subdirectory(scripting)
add_subdirectory(plugins)
add_subdirectory(physics)
add_subdirectory(window)
add_subdirectory(render)
add_subdirectory(audio)
add_subdirectory(save)
add_subdirectory(network)
add_subdirectory(core)
if(NOT ANDROID)
    add_subdirectory(editor)
    add_subdirectory(project_check)
    add_subdirectory(app)
endif()
