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

#ifndef OPENPBR_DIFFUSE_SPECULAR_H
#define OPENPBR_DIFFUSE_SPECULAR_H

struct OpenPBR_DiffuseSpecular
{
    vec3 diffuse;
    vec3 specular;
};

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_make_diffuse_specular(const vec3 diffuse, const vec3 specular)
{
    OpenPBR_DiffuseSpecular result;
    result.diffuse = diffuse;
    result.specular = specular;
    return result;
}

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_make_zero_diffuse_specular()
{
    return openpbr_make_diffuse_specular(vec3(0.0f), vec3(0.0f));
}

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_make_one_diffuse_specular()
{
    return openpbr_make_diffuse_specular(vec3(1.0f), vec3(1.0f));
}

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_make_diffuse_specular_from_diffuse(const vec3 diffuse)
{
    return openpbr_make_diffuse_specular(diffuse, vec3(0.0f));
}

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_make_diffuse_specular_from_specular(const vec3 specular)
{
    return openpbr_make_diffuse_specular(vec3(0.0f), specular);
}

INLINE_FUNCTION bool openpbr_is_nonzero_diffuse_specular(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) diffuse_specular)
{
    return any(notEqual(diffuse_specular.diffuse, vec3(0.0f))) || any(notEqual(diffuse_specular.specular, vec3(0.0f)));
}

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_scale_diffuse_specular(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) diffuse_specular,
                                                                       const vec3 scale)
{
    return openpbr_make_diffuse_specular(diffuse_specular.diffuse * scale, diffuse_specular.specular * scale);
}

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_scale_diffuse_specular(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) diffuse_specular,
                                                                       const float scale)
{
    return openpbr_make_diffuse_specular(diffuse_specular.diffuse * scale, diffuse_specular.specular * scale);
}

INLINE_FUNCTION vec3 openpbr_extract_diffuse_from_diffuse_specular(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) diffuse_specular)
{
    return diffuse_specular.diffuse;
}

INLINE_FUNCTION vec3 openpbr_extract_specular_from_diffuse_specular(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) diffuse_specular)
{
    return diffuse_specular.specular;
}

INLINE_FUNCTION void openpbr_set_diffuse_in_diffuse_specular(ADDRESS_SPACE_THREAD INOUT(OpenPBR_DiffuseSpecular) diffuse_specular, const vec3 diffuse)
{
    diffuse_specular.diffuse = diffuse;
}

INLINE_FUNCTION void openpbr_set_specular_in_diffuse_specular(ADDRESS_SPACE_THREAD INOUT(OpenPBR_DiffuseSpecular) diffuse_specular,
                                                              const vec3 specular)
{
    diffuse_specular.specular = specular;
}

INLINE_FUNCTION vec3 openpbr_get_sum_of_diffuse_specular(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) diffuse_specular)
{
    return diffuse_specular.diffuse + diffuse_specular.specular;
}

INLINE_FUNCTION OpenPBR_DiffuseSpecular openpbr_add_diffuse_specular(ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) a,
                                                                     ADDRESS_SPACE_THREAD CONST_REF(OpenPBR_DiffuseSpecular) b)
{
    return openpbr_make_diffuse_specular(a.diffuse + b.diffuse, a.specular + b.specular);
}

#endif  // !OPENPBR_DIFFUSE_SPECULAR_H
