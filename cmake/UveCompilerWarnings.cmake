# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# uve_set_warnings(target) applies UVE's standard strict-warnings-as-errors
# policy to a single CMake target. Verified compatible with both GCC and
# Clang. Applied uniformly to every uve_* target so the whole codebase shares
# one quality bar.

function(uve_set_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wold-style-cast
        -Wcast-align
        -Werror
    )
endfunction()
