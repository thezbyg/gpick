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
#include <algorithm>
using namespace math;
BOOST_AUTO_TEST_SUITE(octreeColorQuantization)
template<typename T>
constexpr bool between(T value, T min, T max) {
	return value >= min && value <= max;
}
static uint8_t toUint8(float value) {
	return static_cast<uint8_t>(std::max(std::min(static_cast<int>(value * 256), 255), 0));
}
BOOST_AUTO_TEST_CASE(maxNodes) {
	OctreeColorQuantization octree;
	Color color;
	for (int red = 0; red < 256; red += 2) {
		for (int green = 0; green < 256; green += 2) {
			for (int blue = 0; blue < 256; blue += 2) {
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
	BOOST_CHECK_EQUAL(octree.size(), 200u);
}
BOOST_AUTO_TEST_CASE(merge) {
	OctreeColorQuantization octree1, octree2;
	Color color;
	for (int red = 0; red < 256; red += 8) {
		for (int green = 0; green < 256; green += 8) {
			for (int blue = 0; blue < 128; blue += 8) {
				std::array<uint8_t, 3> position = { static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue) };
				color.xyz.x = red * (1 / 255.0f);
				color.xyz.y = green * (1 / 255.0f);
				color.xyz.z = blue * (1 / 255.0f);
				color.alpha = 1.0f;
				color.linearRgbInplace();
				octree1.add(color, position);
				position = { static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(128 + blue) };
				color.xyz.x = red * (1 / 255.0f);
				color.xyz.y = green * (1 / 255.0f);
				color.xyz.z = 0.5f + blue * (1 / 255.0f);
				color.alpha = 1.0f;
				color.linearRgbInplace();
				octree2.add(color, position);
			}
		}
	}
	BOOST_CHECK_PREDICATE(between<size_t>, (octree1.size())(OctreeColorQuantization::maxNodesPerLevel / 2)(OctreeColorQuantization::maxNodesPerLevel));
	BOOST_CHECK_PREDICATE(between<size_t>, (octree2.size())(OctreeColorQuantization::maxNodesPerLevel / 2)(OctreeColorQuantization::maxNodesPerLevel));
	octree1.reduce(100);
	octree2.reduce(100);
	BOOST_CHECK_EQUAL(octree1.size(), 100u);
	BOOST_CHECK_EQUAL(octree2.size(), 100u);
	OctreeColorQuantization mergedOctree;
	octree1.visit([&](const float sum[3], size_t pixels) {
		Color color(sum[0] / pixels, sum[1] / pixels, sum[2] / pixels, 1.0f);
		Color nonLinearColor = color.nonLinearRgb();
		std::array<uint8_t, 3> position = { toUint8(nonLinearColor.red), toUint8(nonLinearColor.green), toUint8(nonLinearColor.blue) };
		mergedOctree.add(color, pixels, position);
	});
	octree2.visit([&](const float sum[3], size_t pixels) {
		Color color(sum[0] / pixels, sum[1] / pixels, sum[2] / pixels, 1.0f);
		Color nonLinearColor = color.nonLinearRgb();
		std::array<uint8_t, 3> position = { toUint8(nonLinearColor.red), toUint8(nonLinearColor.green), toUint8(nonLinearColor.blue) };
		mergedOctree.add(color, pixels, position);
	});
	BOOST_CHECK_EQUAL(mergedOctree.size(), 200u);
}
BOOST_AUTO_TEST_SUITE_END()
