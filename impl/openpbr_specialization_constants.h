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

#ifndef OPENPBR_SPECIALIZATION_CONSTANTS_H
#define OPENPBR_SPECIALIZATION_CONSTANTS_H

// Specialization constant 0, 1 are reserved for scene-related information
// TODO: Decouple the OpenPBR BSDF specialization constants from the scene ones.
OPENPBR_DECLARE_SPECIALIZATION_CONSTANT(2, EnableSheenAndCoat, true);
OPENPBR_DECLARE_SPECIALIZATION_CONSTANT(3, EnableDispersion, true);
OPENPBR_DECLARE_SPECIALIZATION_CONSTANT(4, EnableTranslucency, true);
OPENPBR_DECLARE_SPECIALIZATION_CONSTANT(5, EnableMetallic, true);

#endif  // !OPENPBR_SPECIALIZATION_CONSTANTS_H
