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
#include "math/BinaryTreeQuantization.h"
#include "math/Algorithms.h"
#include <algorithm>
using namespace math;
BOOST_AUTO_TEST_SUITE(binaryTreeQuantization)
template<typename T>
constexpr bool between(T value, T min, T max) {
	return value >= min && value <= max;
}
BOOST_AUTO_TEST_CASE(reduceByCount) {
	BinaryTreeQuantization<float> tree;
	for (int value = 0; value < 128; ++value) {
		tree.add(value * (1 / 128.0f));
	}
	tree.reduce(15);
	BOOST_CHECK_EQUAL(tree.size(), 15);
}
BOOST_AUTO_TEST_CASE(reduceToOneValue) {
	BinaryTreeQuantization<float> tree;
	for (int value = 0; value < 128; ++value) {
		tree.add(value * (1 / 128.0f));
	}
	tree.reduce(1);
	BOOST_CHECK_EQUAL(tree.size(), 1);
}
BOOST_AUTO_TEST_CASE(grouping) {
	BinaryTreeQuantization<float> tree;
	for (int value = 0; value < 1024; ++value) {
		float v = value * (1 / 1024.0f);
		tree.add(static_cast<float>(v + std::sin(v * math::PI * 8) * 0.05f));
	}
	tree.reduce(4);
	BOOST_CHECK_EQUAL(tree.size(), 4);
	struct {
		float min, max;
	} expected[5] = {
		{ 0.12f, 0.13f },
		{ 0.37f, 0.38f },
		{ 0.62f, 0.63f },
		{ 0.87f, 0.88f },
	};
	size_t index = 0;
	tree.visit([&index, expected](float sum, size_t count) {
		BOOST_CHECK_PREDICATE(between<float>, (sum / count)(expected[index].min)(expected[index].max));
		++index;
	});
}
BOOST_AUTO_TEST_CASE(minDistance) {
	BinaryTreeQuantization<float> tree;
	for (int value = 0; value < 1024; ++value) {
		tree.add(value * (1 / 1024.0f));
	}
	BOOST_CHECK_PREDICATE(between<size_t>, (tree.size())(BinaryTreeQuantization<float>::maxNodesPerLevel / 2)(BinaryTreeQuantization<float>::maxNodesPerLevel));
	tree.reduceByMinDistance(1 / 16.0f);
	BOOST_CHECK_EQUAL(tree.size(), 16);
	tree.reduceByMinDistance(1);
	BOOST_CHECK_EQUAL(tree.size(), 1);
}
BOOST_AUTO_TEST_CASE(minDistanceGrouping) {
	BinaryTreeQuantization<float> tree;
	tree.add(0.10f);
	tree.add(0.11f);
	tree.add(0.12f);
	tree.add(0.30f);
	tree.add(0.31f);
	tree.add(0.32f);
	tree.add(0.50f);
	tree.add(0.51f);
	tree.add(0.52f);
	tree.add(0.70f);
	tree.add(0.71f);
	tree.add(0.72f);
	tree.add(0.90f);
	tree.add(0.91f);
	tree.add(0.92f);
	tree.reduceByMinDistance(1 / 10.0f);
	BOOST_CHECK_EQUAL(tree.size(), 5);
}
BOOST_AUTO_TEST_CASE(minDistanceReductionToSingleValue) {
	BinaryTreeQuantization<float> tree;
	for (int value = 0; value < 128; ++value) {
		tree.add(value * (1 / 128.0f));
	}
	tree.reduceByMinDistance(1);
	BOOST_CHECK_EQUAL(tree.size(), 1);
}
BOOST_AUTO_TEST_SUITE_END()
