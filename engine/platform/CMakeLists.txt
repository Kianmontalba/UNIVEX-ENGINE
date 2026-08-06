# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#
# uve_platform is header-only this increment (compiler/OS macros only), so it
# is declared as an INTERFACE library: nothing to compile, but consumers still
# get the include path and can be linked against uniformly like every other
# uve_* target.

add_library(uve_platform INTERFACE)
target_include_directories(uve_platform INTERFACE include)
target_compile_features(uve_platform INTERFACE cxx_std_20)
