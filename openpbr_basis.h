/***************************************************************************
 * Copyright 2026 Adobe
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#ifndef OPENPBR_BASIS_H
#define OPENPBR_BASIS_H

#include "impl/openpbr_math.h"

// This class stores an orthonormal basis with a z-up local space.
// It can be right-handed (handedness +1) or left-handed (handedness -1).

struct OpenPBR_Basis
{
    vec3 t, b, n;
};

INLINE_FUNCTION OpenPBR_Basis openpbr_make_identity_basis()
{
    OpenPBR_Basis basis;

    basis.t = vec3(1.0f, 0.0f, 0.0f);
    basis.b = vec3(0.0f, 1.0f, 0.0f);
    basis.n = vec3(0.0f, 0.0f, 1.0f);

    return basis;
}

// The input normal needs to be unit length.
INLINE_FUNCTION OpenPBR_Basis openpbr_make_basis(const vec3 normal)
{
    ASSERT(openpbr_is_normalized(normal), "[openpbr_make_basis] unnormalized input normal");
    OpenPBR_Basis basis;
    basis.n = normal;

    // The code below uses the algorithm from "Building an Orthonormal OpenPBR_Basis, Revisited"
    // (http://jcgt.org/published/0006/01/01/).

    const float nz_sign = basis.n.z < 0.0f ? -1.0f : 1.0f;
    const float a = -1.0f / (nz_sign + basis.n.z);
    const float b = basis.n.x * basis.n.y * a;

    basis.t = vec3(b, nz_sign + basis.n.y * basis.n.y * a, -basis.n.y);
    basis.b = vec3(1.0f + nz_sign * basis.n.x * basis.n.x * a, nz_sign * b, -nz_sign * basis.n.x);

    return basis;
}

// The input normal needs to be unit length.
INLINE_FUNCTION OpenPBR_Basis openpbr_make_basis(const vec3 normal, const vec3 tangent, const float handedness)
{
    ASSERT(openpbr_is_normalized(normal), "[openpbr_make_basis] unnormalized input normal");
    ASSERT(handedness == 1.0f || handedness == -1.0f, "[openpbr_make_basis] invalid handedness");
    OpenPBR_Basis basis;

    basis.n = normal;
    basis.t = openpbr_fast_normalize(tangent - dot(tangent, basis.n) * basis.n);
    basis.b = cross(basis.n, basis.t) * handedness;

    return basis;
}

// The input normal needs to be unit length.
INLINE_FUNCTION OpenPBR_Basis openpbr_make_basis(const vec3 normal, const vec3 tangent, const vec3 bitangent)
{
    // Orthonormalize the input vectors using the modified Gram-Schmidt process
    // (which is more numerically stable than the classical Gram-Schmidt process).
    // For details, see https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process#Numerical_stability.

    ASSERT(openpbr_is_normalized(normal), "[openpbr_make_basis] unnormalized input normal");

    const vec3 new_tangent = openpbr_fast_normalize(tangent - dot(tangent, normal) * normal);  // make tangent perpendicular to normal
    const vec3 intermediate_bitangent = bitangent - dot(bitangent, normal) * normal;           // make bitangent perpendicular to normal
    const vec3 new_bitangent = openpbr_fast_normalize(intermediate_bitangent - dot(intermediate_bitangent, new_tangent) *
                                                                                   new_tangent);  // make bitangent perpendicular to tangent also

    OpenPBR_Basis basis;

    basis.t = new_tangent;
    basis.b = new_bitangent;
    basis.n = normal;  // would need to be explicitly normalized if function accepted unnormalized normals

    return basis;
}

INLINE_FUNCTION void openpbr_invert_basis(ADDRESS_SPACE_THREAD INOUT(OpenPBR_Basis) basis)
{
    basis.t *= -1.0f;
    basis.b *= -1.0f;
    basis.n *= -1.0f;
}

INLINE_FUNCTION float openpbr_get_basis_handedness(const vec3 normal, const vec3 tangent, const vec3 bitangent)
{
    return dot(cross(tangent, bitangent), normal) >= 0.0f ? 1.0f : -1.0f;
}

INLINE_FUNCTION float openpbr_get_basis_handedness(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_Basis) basis)
{
    return openpbr_get_basis_handedness(basis.n, basis.t, basis.b);
}

// Rotates counterclockwise around the normal.
INLINE_FUNCTION void openpbr_rotate_basis_around_normal(ADDRESS_SPACE_THREAD INOUT(OpenPBR_Basis) basis, vec2 cos_sin, float handedness)
{
    const vec3 rotated_tangent = basis.t * cos_sin.x + handedness * basis.b * cos_sin.y;
    const vec3 rotated_bitangent = basis.b * cos_sin.x - handedness * basis.t * cos_sin.y;
    basis.t = rotated_tangent;
    basis.b = rotated_bitangent;
}

INLINE_FUNCTION void openpbr_rotate_basis_around_normal(ADDRESS_SPACE_THREAD INOUT(OpenPBR_Basis) basis, vec2 cos_sin)
{
    openpbr_rotate_basis_around_normal(basis, cos_sin, openpbr_get_basis_handedness(basis));
}

INLINE_FUNCTION void openpbr_rotate_basis_around_normal(ADDRESS_SPACE_THREAD INOUT(OpenPBR_Basis) basis, float radians)
{
    const float sine = sin(radians);
    const float cosine = cos(radians);
    openpbr_rotate_basis_around_normal(basis, vec2(cosine, sine));
}

INLINE_FUNCTION vec3 openpbr_world_to_local(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_Basis) basis, const vec3 direction)
{
    return vec3(dot(direction, basis.t), dot(direction, basis.b), dot(direction, basis.n));
}

INLINE_FUNCTION vec3 openpbr_local_to_world(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_Basis) basis, const vec3 direction)
{
    return direction.x * basis.t + direction.y * basis.b + direction.z * basis.n;
}

#endif
