// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_tree_uve.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace UVE::Core {
namespace {

constexpr std::size_t kMaximumIdentifierBytesUVE = 128U;

[[nodiscard]] const AnimationTreeNodeUVE* FindNodeUVE(
    const AnimationTreeUVE& tree, const std::uint32_t id) noexcept {
    const auto iterator = std::find_if(tree.nodes.cbegin(), tree.nodes.cend(), [id](const auto& node) {
        return node.id == id;
    });
    return iterator == tree.nodes.cend() ? nullptr : &*iterator;
}

[[nodiscard]] const AnimationClipUVE* FindClipUVE(
    const AnimationTreeUVE& tree, const std::string& clipId) noexcept {
    const auto iterator = std::find_if(tree.clips.cbegin(), tree.clips.cend(), [&clipId](const auto& clip) {
        return clip.clipId == clipId;
    });
    return iterator == tree.clips.cend() ? nullptr : &*iterator;
}

[[nodiscard]] float FindParameterValueUVE(
    const std::vector<AnimationTreeParameterUVE>& parameters, const std::string& parameterId) noexcept {
    const auto iterator = std::find_if(parameters.cbegin(), parameters.cend(), [&parameterId](const auto& parameter) {
        return parameter.parameterId == parameterId;
    });
    return iterator == parameters.cend() ? 0.0F : iterator->value;
}

[[nodiscard]] TransformPoseUVE BlendPoseUVE(const TransformPoseUVE& left,
                                            const TransformPoseUVE& right,
                                            const float weight) noexcept {
    const float factor = std::clamp(weight, 0.0F, 1.0F);
    TransformPoseUVE blended;
    blended.position = left.position * (1.0F - factor) + right.position * factor;
    blended.scale = left.scale * (1.0F - factor) + right.scale * factor;
    blended.rotation = Math::QuaternionUVE{
        left.rotation.x * (1.0F - factor) + right.rotation.x * factor,
        left.rotation.y * (1.0F - factor) + right.rotation.y * factor,
        left.rotation.z * (1.0F - factor) + right.rotation.z * factor,
        left.rotation.w * (1.0F - factor) + right.rotation.w * factor,
    };
    TransformPoseUVE normalized;
    return TryNormalizeTransformPoseUVE(blended, normalized) ? normalized : left;
}

[[nodiscard]] bool UsesInputAUVE(const AnimationTreeNodeKindUVE kind) noexcept {
    return kind != AnimationTreeNodeKindUVE::ClipPlayer;
}

[[nodiscard]] bool UsesInputBUVE(const AnimationTreeNodeKindUVE kind) noexcept {
    return kind == AnimationTreeNodeKindUVE::Blend || kind == AnimationTreeNodeKindUVE::Transition;
}

} // namespace

AnimationTreeValidationResultUVE ValidateAnimationTreeUVE(const AnimationTreeUVE& tree) noexcept {
    if (tree.nodes.empty()) {
        return {AnimationTreeValidationCodeUVE::EmptyTree, 0U, "AnimationTree requires at least one node."};
    }
    if (tree.nodes.size() > AnimationTreeUVE::kMaximumNodesUVE) {
        return {AnimationTreeValidationCodeUVE::CapacityExceeded, 0U,
                "AnimationTree node count exceeds the bounded limit."};
    }
    std::unordered_set<std::uint32_t> nodeIds;
    nodeIds.reserve(tree.nodes.size());
    for (const AnimationTreeNodeUVE& node : tree.nodes) {
        if (node.id == 0U || node.name.empty() || node.name.size() > kMaximumIdentifierBytesUVE ||
            !std::isfinite(node.weight) || node.weight < 0.0F || node.weight > 1.0F ||
            !std::isfinite(node.timeScale) || node.timeScale < 0.0F) {
            return {AnimationTreeValidationCodeUVE::InvalidNode, node.id,
                    "AnimationTree node identity or bounded numeric configuration is invalid."};
        }
        if (!nodeIds.insert(node.id).second) {
            return {AnimationTreeValidationCodeUVE::DuplicateNode, node.id,
                    "AnimationTree node identifiers must be unique."};
        }
    }
    for (const AnimationClipUVE& clip : tree.clips) {
        const AnimationClipValidationResultUVE clipResult = ValidateAnimationClipUVE(clip);
        if (!clipResult.IsValidUVE()) {
            return {AnimationTreeValidationCodeUVE::InvalidClip, 0U,
                    "AnimationTree contains an invalid AnimationClip resource."};
        }
    }
    std::size_t outputCount = 0U;
    for (const AnimationTreeNodeUVE& node : tree.nodes) {
        if (node.kind == AnimationTreeNodeKindUVE::ClipPlayer &&
            (node.clipId.empty() || FindClipUVE(tree, node.clipId) == nullptr)) {
            return {AnimationTreeValidationCodeUVE::UnknownClip, node.id,
                    "AnimationTree ClipPlayer references an unknown clip."};
        }
        if (node.kind == AnimationTreeNodeKindUVE::Parameter &&
            (node.parameterId.empty() || node.parameterId.size() > kMaximumIdentifierBytesUVE)) {
            return {AnimationTreeValidationCodeUVE::InvalidParameter, node.id,
                    "AnimationTree Parameter requires a bounded parameter identifier."};
        }
        if (node.kind == AnimationTreeNodeKindUVE::OutputPose) {
            ++outputCount;
        }
        if (UsesInputAUVE(node.kind) && node.inputA == 0U) {
            return {AnimationTreeValidationCodeUVE::InvalidNode, node.id,
                    "AnimationTree node requires inputA."};
        }
        if (UsesInputBUVE(node.kind) && node.inputB == 0U) {
            return {AnimationTreeValidationCodeUVE::InvalidNode, node.id,
                    "AnimationTree node requires inputB."};
        }
        if (node.inputA != 0U && FindNodeUVE(tree, node.inputA) == nullptr) {
            return {AnimationTreeValidationCodeUVE::UnknownInput, node.id,
                    "AnimationTree inputA references an unknown node."};
        }
        if (node.inputB != 0U && FindNodeUVE(tree, node.inputB) == nullptr) {
            return {AnimationTreeValidationCodeUVE::UnknownInput, node.id,
                    "AnimationTree inputB references an unknown node."};
        }
    }
    if (outputCount == 0U) {
        return {AnimationTreeValidationCodeUVE::MissingOutput, 0U,
                "AnimationTree requires an OutputPose node."};
    }

    std::unordered_map<std::uint32_t, std::uint8_t> visitState;
    visitState.reserve(tree.nodes.size());
    const std::function<bool(const AnimationTreeNodeUVE&)> visit = [&](const AnimationTreeNodeUVE& node) {
        const std::uint8_t state = visitState[node.id];
        if (state == 1U) {
            return false;
        }
        if (state == 2U) {
            return true;
        }
        visitState[node.id] = 1U;
        if ((node.inputA != 0U && !visit(*FindNodeUVE(tree, node.inputA))) ||
            (node.inputB != 0U && !visit(*FindNodeUVE(tree, node.inputB)))) {
            return false;
        }
        visitState[node.id] = 2U;
        return true;
    };
    for (const AnimationTreeNodeUVE& node : tree.nodes) {
        if (!visit(node)) {
            return {AnimationTreeValidationCodeUVE::CycleDetected, node.id,
                    "AnimationTree node inputs must be acyclic."};
        }
    }
    return {AnimationTreeValidationCodeUVE::Valid, 0U, "AnimationTree is valid."};
}

AnimationTreeEvaluationResultUVE EvaluateAnimationTreeUVE(
    const AnimationTreeUVE& tree, const double timeSeconds,
    const std::vector<AnimationTreeParameterUVE>& parameters) {
    AnimationTreeEvaluationResultUVE result;
    if (!ValidateAnimationTreeUVE(tree).IsValidUVE() || !std::isfinite(timeSeconds)) {
        result.message = "AnimationTree evaluation rejected an invalid tree or time.";
        return result;
    }
    const auto output = std::find_if(tree.nodes.cbegin(), tree.nodes.cend(), [](const auto& node) {
        return node.kind == AnimationTreeNodeKindUVE::OutputPose;
    });
    if (output == tree.nodes.cend()) {
        result.message = "AnimationTree has no output node.";
        return result;
    }
    std::unordered_map<std::uint32_t, TransformPoseUVE> cache;
    std::unordered_set<std::uint32_t> evaluating;
    std::function<bool(const AnimationTreeNodeUVE&, double, TransformPoseUVE&)> evaluate =
        [&](const AnimationTreeNodeUVE& node, const double localTime, TransformPoseUVE& outPose) {
            if (const auto cached = cache.find(node.id); cached != cache.end()) {
                outPose = cached->second;
                return true;
            }
            if (!evaluating.insert(node.id).second) {
                return false;
            }
            bool success = true;
            switch (node.kind) {
                case AnimationTreeNodeKindUVE::ClipPlayer: {
                    const AnimationClipUVE* clip = FindClipUVE(tree, node.clipId);
                    success = clip != nullptr && TrySampleAnimationClipUVE(*clip, localTime, true, outPose);
                    break;
                }
                case AnimationTreeNodeKindUVE::Blend: {
                    TransformPoseUVE left;
                    TransformPoseUVE right;
                    success = evaluate(*FindNodeUVE(tree, node.inputA), localTime, left) &&
                              evaluate(*FindNodeUVE(tree, node.inputB), localTime, right);
                    if (success) {
                        outPose = BlendPoseUVE(left, right, node.weight);
                    }
                    break;
                }
                case AnimationTreeNodeKindUVE::Transition: {
                    const float parameter = FindParameterValueUVE(parameters, node.parameterId);
                    const AnimationTreeNodeUVE* selected = parameter > 0.5F
                        ? FindNodeUVE(tree, node.inputB) : FindNodeUVE(tree, node.inputA);
                    success = selected != nullptr && evaluate(*selected, localTime, outPose);
                    break;
                }
                case AnimationTreeNodeKindUVE::TimeScale:
                    success = evaluate(*FindNodeUVE(tree, node.inputA), localTime * node.timeScale, outPose);
                    break;
                case AnimationTreeNodeKindUVE::Parameter:
                case AnimationTreeNodeKindUVE::State:
                case AnimationTreeNodeKindUVE::OneShot:
                case AnimationTreeNodeKindUVE::Sync:
                case AnimationTreeNodeKindUVE::Subtree:
                case AnimationTreeNodeKindUVE::PoseCache:
                case AnimationTreeNodeKindUVE::OutputPose:
                    success = evaluate(*FindNodeUVE(tree, node.inputA), localTime, outPose);
                    break;
            }
            evaluating.erase(node.id);
            if (success) {
                cache.emplace(node.id, outPose);
                ++result.evaluatedNodeCount;
            }
            return success;
        };
    result.usedOutputNode = evaluate(*output, timeSeconds, result.pose);
    result.message = result.usedOutputNode ? "AnimationTree evaluated successfully." :
                                             "AnimationTree evaluation failed.";
    return result;
}

} // namespace UVE::Core
