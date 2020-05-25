/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Base.h>
#include <pylon/Node.h>

namespace ply {

inline Float4x4 toFloat4x4(const pylon::Node& aMatrix) {
    Float4x4 result;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            result[col][row] = aMatrix[col][row].numeric<float>();
        }
    }
    return result;
}

inline Float4 toFloat4(const pylon::Node& aVector) {
    Float4 result;
    for (int i = 0; i < 4; i++) {
        result[i] = aVector[i].numeric<float>();
    }
    return result;
}

inline Float3 toFloat3(const pylon::Node& aVector) {
    Float3 result;
    for (int i = 0; i < 3; i++) {
        result[i] = aVector[i].numeric<float>();
    }
    return result;
}

inline Float2 toFloat2(const pylon::Node& aVector) {
    Float2 result;
    for (int i = 0; i < 2; i++) {
        result[i] = aVector[i].numeric<float>();
    }
    return result;
}

inline QuatPos toQuatPos(const pylon::Node& aQuatPos) {
    QuatPos result;
    result.quat = toFloat4(aQuatPos["quat"]).asQuaternion();
    result.pos = toFloat3(aQuatPos["pos"]);
    return result;
}

} // namespace ply
