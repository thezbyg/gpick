/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the software author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <boost/test/unit_test.hpp>
#include "math/OctreeColorQuantization.h"
#include <iostream>
using namespace math;
BOOST_AUTO_TEST_SUITE(octreeColorQuantization)
template<typename T>
constexpr bool between(T value, T min, T max) {
	return value >= min && value <= max;
}
BOOST_AUTO_TEST_CASE(max) {
	OctreeColorQuantization octree;
	Color color;
	for (int red = 0; red < 256; red += 4) {
		for (int green = 0; green < 256; green += 4) {
			for (int blue = 0; blue < 256; blue += 4) {
				std::array<uint8_t, 3> position = { static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue) };
				color.xyz.x = red * (1 / 255.0f);
				color.xyz.y = green * (1 / 255.0f);
				color.xyz.z = blue * (1 / 255.0f);
				color.alpha = 1.0f;
				color.linearRgbInplace();
				octree.add(color, position);
			}
		}
	}
	BOOST_CHECK_PREDICATE(between<size_t>, (octree.size())(OctreeColorQuantization::maxNodesPerLevel / 2)(OctreeColorQuantization::maxNodesPerLevel));
	octree.reduce(200);
	BOOST_CHECK_EQUAL(octree.size(), 200);
}
BOOST_AUTO_TEST_SUITE_END()
