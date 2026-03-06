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

#ifndef OPENPBR_REFLECTION_TRANSMISSION_COEFFICIENT_STRUCTS_H
#define OPENPBR_REFLECTION_TRANSMISSION_COEFFICIENT_STRUCTS_H

// The reflection/transmission structs below are used by the reflection and transmission coefficient classes
// to return multiple coefficients and probabilities together. This allows the classes to avoid redundant
// calculations that would occur if computing and returning the reflection and transmission coefficients separately.

struct OpenPBR_AllCoefficients
{
    vec3 reflection_coefficient;
    vec3 transmission_coefficient;
};

struct OpenPBR_AllCoefficientsAndProbabilities
{
    vec3 reflection_coefficient;
    vec3 transmission_coefficient;
    float reflection_probability;
    float transmission_probability;
};

#endif  // !OPENPBR_REFLECTION_TRANSMISSION_COEFFICIENT_STRUCTS_H
