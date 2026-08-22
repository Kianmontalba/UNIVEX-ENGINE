// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/audio/audio_mixer_group_uve.h"

#include <limits>
#include <string>

#include <gtest/gtest.h>

namespace UVE::Audio::Tests {
namespace {

TEST(AudioMixerGroupUVETest, SaturatingAddAudioMixerSourceCountUVE_ClampsAtSizeMaximum) {
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    EXPECT_EQ(SaturatingAddAudioMixerSourceCountUVE(maximum - 1U, 1U), maximum);
    EXPECT_EQ(SaturatingAddAudioMixerSourceCountUVE(maximum - 1U, 2U), maximum);
    EXPECT_EQ(SaturatingAddAudioMixerSourceCountUVE(4U, 3U), 7U);
}

TEST(AudioMixerGroupUVETest, EvaluateAudioMixParametersUVE_ComposesFiniteFinalValues) {
    AudioMixParametersUVE parameters;
    ASSERT_TRUE(EvaluateAudioMixParametersUVE(0.8F, 1.5F, 0.5F, 2.0F, 0.25F, parameters));
    EXPECT_EQ(parameters, (AudioMixParametersUVE{0.1F, 3.0F}));
    ASSERT_TRUE(EvaluateAudioMixParametersUVE(4.0F, 1.0F, 1.0F, 1.0F, 1.0F, parameters));
    EXPECT_EQ(parameters, (AudioMixParametersUVE{1.0F, 1.0F}));
}

TEST(AudioMixerGroupUVETest, EvaluateAudioMixParametersUVE_RejectsInvalidInputsAtomically) {
    const AudioMixParametersUVE original{0.25F, 1.5F};
    AudioMixParametersUVE parameters = original;
    EXPECT_FALSE(EvaluateAudioMixParametersUVE(-1.0F, 1.0F, 1.0F, 1.0F, 1.0F, parameters));
    EXPECT_EQ(parameters, original);
    EXPECT_FALSE(EvaluateAudioMixParametersUVE(1.0F, 1.0F, 1.0F, 0.1F, 1.0F, parameters));
    EXPECT_EQ(parameters, original);
    EXPECT_FALSE(EvaluateAudioMixParametersUVE(1.0F, 1.0F, 1.0F, 1.0F, 2.0F, parameters));
    EXPECT_EQ(parameters, original);
    EXPECT_FALSE(EvaluateAudioMixParametersUVE(1.0F, std::numeric_limits<float>::max(),
                                                1.0F, 4.0F, 1.0F, parameters));
    EXPECT_EQ(parameters, original);
}

TEST(AudioMixerGroupUVETest, MasterAlwaysExistsAndInvalidDefinitionsAreRejected) {
    AudioMixerGroupUVE mixer;

    EXPECT_TRUE(mixer.HasGroupUVE(kMasterAudioMixerGroupNameUVE));
    EXPECT_FALSE(mixer.RemoveGroupUVE(kMasterAudioMixerGroupNameUVE));
    EXPECT_FALSE(mixer.RegisterGroupUVE("", 1.0F, 1.0F));
    EXPECT_FALSE(mixer.RegisterGroupUVE("SFX", -0.1F, 1.0F));
    EXPECT_FALSE(mixer.RegisterGroupUVE("SFX", 1.0F, 0.0F));
    EXPECT_TRUE(mixer.RegisterGroupUVE("SFX", 0.5F, 1.5F));
    EXPECT_FALSE(mixer.RegisterGroupUVE("SFX", 1.0F, 1.0F));
    EXPECT_TRUE(mixer.HasGroupUVE("SFX"));
}

TEST(AudioMixerGroupUVETest, InvalidLookupNamesAreRejectedWithoutChangingDefaults) {
    AudioMixerGroupUVE mixer;
    const std::string oversizedName(kMaximumAudioMixerGroupNameBytesUVE + 1U, 'X');
    const std::string nulName = std::string("SFX") + '\0' + "Hidden";

    for (const std::string& name : {oversizedName, nulName}) {
        EXPECT_FALSE(mixer.HasGroupUVE(name));
        EXPECT_FALSE(mixer.RemoveGroupUVE(name));
        EXPECT_FALSE(mixer.SetGroupVolumeUVE(name, 0.5F));
        EXPECT_FALSE(mixer.SetGroupPitchUVE(name, 1.5F));
        EXPECT_FALSE(mixer.AttachSourceUVE(name));
        EXPECT_FALSE(mixer.DetachSourceUVE(name));
        EXPECT_FLOAT_EQ(mixer.GetGroupVolumeUVE(name), 1.0F);
        EXPECT_FLOAT_EQ(mixer.GetGroupPitchUVE(name), 1.0F);
        EXPECT_EQ(mixer.ResolveGroupNameUVE(name), kMasterAudioMixerGroupNameUVE);
    }
}

TEST(AudioMixerGroupUVETest, SourceRoutingAndDiagnosticsAreCopiedAndDeterministic) {
    AudioMixerGroupUVE mixer;
    ASSERT_TRUE(mixer.RegisterGroupUVE("Music", 0.75F, 0.8F));
    ASSERT_TRUE(mixer.RegisterGroupUVE("SFX", 0.5F, 1.25F));
    ASSERT_TRUE(mixer.AttachSourceUVE("SFX"));
    ASSERT_TRUE(mixer.AttachSourceUVE("Music"));
    ASSERT_TRUE(mixer.AttachSourceUVE(kMasterAudioMixerGroupNameUVE));

    const AudioMixerDiagnosticsUVE diagnostics = mixer.GetDiagnosticsUVE();
    ASSERT_EQ(diagnostics.groupCount, 3U);
    ASSERT_EQ(diagnostics.inspectedGroupCount, 3U);
    ASSERT_EQ(diagnostics.routedSourceCount, 3U);
    ASSERT_FALSE(diagnostics.truncated);
    ASSERT_EQ(diagnostics.groups.size(), 3U);
    EXPECT_EQ(diagnostics.groups[0].name, kMasterAudioMixerGroupNameUVE);
    EXPECT_EQ(diagnostics.groups[1].name, "Music");
    EXPECT_EQ(diagnostics.groups[2].name, "SFX");
    EXPECT_EQ(diagnostics.groups[2].sourceCount, 1U);

    const AudioMixerDiagnosticsUVE capped = mixer.GetDiagnosticsUVE(2U);
    EXPECT_EQ(capped.groupCount, 3U);
    EXPECT_EQ(capped.inspectedGroupCount, 2U);
    EXPECT_EQ(capped.groups.size(), 2U);
    EXPECT_TRUE(capped.truncated);
    EXPECT_EQ(capped.routedSourceCount, 3U);

    EXPECT_FALSE(mixer.RemoveGroupUVE("SFX"));
    EXPECT_TRUE(mixer.DetachSourceUVE("SFX"));
    EXPECT_TRUE(mixer.RemoveGroupUVE("SFX"));
    EXPECT_EQ(mixer.ResolveGroupNameUVE("SFX"), kMasterAudioMixerGroupNameUVE);
}

TEST(AudioMixerGroupUVETest, MultiplierUpdatesRequireExistingGroupsAndValidRanges) {
    AudioMixerGroupUVE mixer;
    ASSERT_TRUE(mixer.RegisterGroupUVE("Voice", 1.0F, 1.0F));

    EXPECT_TRUE(mixer.SetGroupVolumeUVE("Voice", 0.25F));
    EXPECT_TRUE(mixer.SetGroupPitchUVE("Voice", 2.0F));
    EXPECT_FLOAT_EQ(mixer.GetGroupVolumeUVE("Voice"), 0.25F);
    EXPECT_FLOAT_EQ(mixer.GetGroupPitchUVE("Voice"), 2.0F);
    EXPECT_FALSE(mixer.SetGroupVolumeUVE("Voice", 1.1F));
    EXPECT_FALSE(mixer.SetGroupPitchUVE("Voice", 0.1F));
    EXPECT_FALSE(mixer.SetGroupVolumeUVE("Missing", 0.5F));
    EXPECT_FLOAT_EQ(mixer.GetGroupVolumeUVE("Missing"), 1.0F);
    EXPECT_FLOAT_EQ(mixer.GetGroupPitchUVE("Missing"), 1.0F);
}

} // namespace
} // namespace UVE::Audio::Tests
