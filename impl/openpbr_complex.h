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

#ifndef OPENPBR_COMPLEX_H
#define OPENPBR_COMPLEX_H

// Complex arithmetic functions

// A complex number is represented by a vec2
// The vec2's first component represents the real component
// The vec2's second component represents the imaginary component
// The constructor complex(x, y) can then be used to create a complex number
#define openpbr_complex vec2

// Complex negate
openpbr_complex openpbr_complex_negate(const openpbr_complex z)
{
    return -z;
}

// Function to create a complex exponential e^(i * theta)
openpbr_complex openpbr_complex_exp_i(const float theta)
{
    return openpbr_complex(cos(theta), sin(theta));
}

// Complex addition: z1 + z2
openpbr_complex openpbr_complex_add(const openpbr_complex z1, const openpbr_complex z2)
{
    return z1 + z2;
}

// Complex subtraction: z1 - z2
openpbr_complex openpbr_complex_subtract(const openpbr_complex z1, const openpbr_complex z2)
{
    return z1 - z2;
}

// Complex multiplication with scalar: scalar * z
openpbr_complex openpbr_complex_scalar_multiply(const float scalar, const openpbr_complex z)
{
    return openpbr_complex(scalar * z.x, scalar * z.y);
}

// Complex multiplication: z1 * z2
openpbr_complex openpbr_complex_multiply(const openpbr_complex z1, const openpbr_complex z2)
{
    return openpbr_complex(z1.x * z2.x - z1.y * z2.y, z1.x * z2.y + z1.y * z2.x);
}

// Complex division: z1 / z2
openpbr_complex openpbr_complex_divide(const openpbr_complex z1, const openpbr_complex z2, const float min_denom_mag, const openpbr_complex fallback)
{
    const float denom = dot(z2, z2);  // z2.x * z2.x + z2.y * z2.y
    if (denom < min_denom_mag * min_denom_mag)
    {
        // Return the fallback to avoid division by zero
        return fallback;
    }
    // Precompute reciprocal denominators to minimize number of divisions
    const float rcp_denom = 1.0f / denom;
    return openpbr_complex((z1.x * z2.x + z1.y * z2.y) * rcp_denom, (z1.y * z2.x - z1.x * z2.y) * rcp_denom);
}

// Complex square root function
openpbr_complex openpbr_complex_sqrt(const openpbr_complex z)
{
    const float r = length(z);

    const float a = sqrt(max((r + z.x) * 0.5f, 0.0f));
    const float b = openpbr_sign_nonzero(z.y) * sqrt(max((r - z.x) * 0.5f, 0.0f));

    return openpbr_complex(a, b);
}

// Complex magnitude squared: |z|^2
float openpbr_complex_magnitude_squared(const openpbr_complex z)
{
    return dot(z, z);  // z.x * z.x + z.y * z.y
}

#endif  // !OPENPBR_COMPLEX_H
