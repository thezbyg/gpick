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

#include "OctreeColorQuantization.h"
#include <algorithm>
#include <cstring>
#include <new>
namespace math {
using Node = OctreeColorQuantization::Node;
template<typename... Args>
Node *newNode(OctreeColorQuantization::Allocator &allocator, std::vector<Node *> &freeNodes, Args &&...args) {
	Node *node;
	if (freeNodes.size() > 0) {
		node = freeNodes.back();
		freeNodes.pop_back();
	} else {
		node = allocator.allocate(1);
	}
	return new (node) Node(std::forward<Args>(args)...);
}
template<size_t MaxNodesPerLevel>
constexpr size_t maxTotalNodes(size_t value, uint8_t depth) {
	if (depth == 0)
		return std::min(value, MaxNodesPerLevel);
	return std::min(value, MaxNodesPerLevel) + maxTotalNodes<MaxNodesPerLevel>(value * 8, --depth);
}
static uint8_t toIndex(uint8_t value, uint8_t depth) {
	return (value >> (7 - depth)) & 1;
}
OctreeColorQuantization::Node::Node() noexcept:
	m_children { nullptr },
	m_pixels(0),
	m_colorSum { 0 } {
}
OctreeColorQuantization::Node::Node(const Node &node, uint8_t depth, OctreeColorQuantization &ocq):
	m_children { nullptr },
	m_pixels(node.m_pixels),
	m_colorSum { node.m_colorSum[0], node.m_colorSum[1], node.m_colorSum[2] } {
	if (m_pixels > 0)
		return;
	for (uint8_t i = 0; i < children; i++) {
		if (node.m_children[i]) {
			m_children[i] = newNode(ocq.m_allocator, ocq.m_freeNodes, *node.m_children[i], depth + 1, ocq);
			if (depth < maxDepth - 1)
				ocq.m_levels[depth].push_back(m_children[i]);
		} else {
			m_children[i] = nullptr;
		}
	}
}
void OctreeColorQuantization::Node::clear() {
	m_pixels = 0;
	m_colorSum[0] = 0;
	m_colorSum[1] = 0;
	m_colorSum[2] = 0;
	std::memset(m_children, 0, sizeof(m_children));
}
size_t OctreeColorQuantization::Node::leafs() const {
	size_t result = isLeaf() ? 1 : 0;
	for (uint8_t i = 0; i < children; i++) {
		if (m_children[i])
			result += m_children[i]->leafs();
	}
	return result;
}
uint8_t OctreeColorQuantization::Node::removeLeafs(OctreeColorQuantization &ocq) {
	bool wasLeaf = isLeaf();
	uint8_t result = 0;
	for (uint8_t i = 0; i < children; i++) {
		if (!m_children[i])
			continue;
		auto &node = *m_children[i];
		ocq.m_freeNodes.push_back(m_children[i]);
		m_children[i] = nullptr;
		m_pixels += node.m_pixels;
		m_colorSum[0] += node.m_colorSum[0];
		m_colorSum[1] += node.m_colorSum[1];
		m_colorSum[2] += node.m_colorSum[2];
		++result;
	}
	if (!wasLeaf && isLeaf())
		--result;
	return result;
}
uint8_t OctreeColorQuantization::Node::reduceLeafs(uint8_t reduceBy, OctreeColorQuantization &ocq) {
	std::array<uint8_t, children> indexes;
	uint8_t have = 0;
	for (uint8_t i = 0; i < children; i++) {
		if (m_children[i]) {
			indexes[have++] = i;
		}
	}
	if (have <= reduceBy + 1)
		return removeLeafs(ocq);
	uint8_t reduced = 0;
	while (reduced < reduceBy) {
		std::sort(indexes.begin(), indexes.begin() + have, [this](uint8_t a, uint8_t b) {
			return m_children[a]->m_pixels > m_children[b]->m_pixels;
		});
		uint8_t sourceIndex = indexes[have - 1];
		uint8_t destinationIndex = indexes[have - 2];
		auto &sourceNode = *m_children[sourceIndex];
		auto &destinationNode = *m_children[destinationIndex];
		destinationNode.m_pixels += sourceNode.m_pixels;
		destinationNode.m_colorSum[0] += sourceNode.m_colorSum[0];
		destinationNode.m_colorSum[1] += sourceNode.m_colorSum[1];
		destinationNode.m_colorSum[2] += sourceNode.m_colorSum[2];
		ocq.m_freeNodes.push_back(m_children[sourceIndex]);
		m_children[sourceIndex] = nullptr;
		++reduced;
		--have;
	}
	return reduced;
}
void OctreeColorQuantization::Node::add(const Color &color, const Position position, uint8_t depth, OctreeColorQuantization &ocq) {
	if (ocq.m_leafs + 1 >= maxNodesPerLevel)
		ocq.reduce(maxNodesPerLevel / 2, false);
	if (depth == maxDepth || isLeaf()) {
		if (m_pixels == 0)
			ocq.m_leafs++;
		m_pixels++;
		m_colorSum[0] += color.xyz.x;
		m_colorSum[1] += color.xyz.y;
		m_colorSum[2] += color.xyz.z;
		return;
	}
	uint8_t i = toIndex(position[0], depth) | (toIndex(position[1], depth) << 1) | (toIndex(position[2], depth) << 2);
	if (!m_children[i]) {
		m_children[i] = newNode(ocq.m_allocator, ocq.m_freeNodes);
		if (depth < maxDepth - 1)
			ocq.m_levels[depth].push_back(m_children[i]);
	}
	m_children[i]->add(color, position, depth + 1, ocq);
}
void OctreeColorQuantization::Node::add(const Color &color, size_t pixels, const Position position, uint8_t depth, OctreeColorQuantization &ocq) {
	if (ocq.m_leafs + 1 >= maxNodesPerLevel)
		ocq.reduce(maxNodesPerLevel / 2, false);
	if (depth == maxDepth || isLeaf()) {
		if (m_pixels == 0)
			ocq.m_leafs++;
		m_pixels += pixels;
		m_colorSum[0] += color.xyz.x * pixels;
		m_colorSum[1] += color.xyz.y * pixels;
		m_colorSum[2] += color.xyz.z * pixels;
		return;
	}
	uint8_t i = toIndex(position[0], depth) | (toIndex(position[1], depth) << 1) | (toIndex(position[2], depth) << 2);
	if (!m_children[i]) {
		m_children[i] = newNode(ocq.m_allocator, ocq.m_freeNodes);
		if (depth < maxDepth - 1)
			ocq.m_levels[depth].push_back(m_children[i]);
	}
	m_children[i]->add(color, pixels, position, depth + 1, ocq);
}
bool OctreeColorQuantization::Node::isLeaf() const {
	return m_pixels > 0;
}
size_t OctreeColorQuantization::Node::totalPixels() const {
	size_t result = m_pixels;
	for (uint8_t i = 0; i < children; i++) {
		if (!m_children[i])
			continue;
		result += m_children[i]->totalPixels();
	}
	return result;
}
OctreeColorQuantization::OctreeColorQuantization():
	m_leafs(0),
	m_memory(maxTotalNodes<maxNodesPerLevel>(1, maxDepth) * sizeof(Node)),
	m_bufferResource(m_memory.data(), m_memory.size(), std::pmr::null_memory_resource()),
	m_allocator(&m_bufferResource) {
}
OctreeColorQuantization::OctreeColorQuantization(const OctreeColorQuantization &ocq):
	m_leafs(ocq.m_leafs),
	m_memory(maxTotalNodes<maxNodesPerLevel>(1, maxDepth) * sizeof(Node)),
	m_bufferResource(m_memory.data(), m_memory.size(), std::pmr::null_memory_resource()),
	m_allocator(&m_bufferResource),
	m_root(ocq.m_root, 0, *this) {
}
void OctreeColorQuantization::add(const Color &color, const Position position) {
	m_root.add(color, position, 0, *this);
}
void OctreeColorQuantization::add(const Color &color, size_t pixels, const Position position) {
	m_root.add(color, pixels, position, 0, *this);
}
void OctreeColorQuantization::clear() {
	m_root.clear();
	m_bufferResource.release();
	for (auto &level: m_levels)
		level.clear();
	m_freeNodes.clear();
	m_leafs = 0;
}
void OctreeColorQuantization::rebuildLevel(uint8_t level) {
	if (level == 0) {
		for (uint8_t i = 0; i < children; i++) {
			if (m_root.m_children[i])
				m_levels[level].push_back(m_root.m_children[i]);
		}
	} else {
		for (auto node: m_levels[level - 1]) {
			for (uint8_t i = 0; i < children; i++) {
				if (node->m_children[i])
					m_levels[level].push_back(node->m_children[i]);
			}
		}
	}
}
void OctreeColorQuantization::reduce(size_t numberOfColors, bool accurate) {
	if (!accurate)
		numberOfColors += 8;
	if (m_leafs <= numberOfColors)
		return;
	for (int i = static_cast<int>(maxDepth - 2); i >= 0; --i) {
		std::vector<Node *> &level = m_levels[i];
		if (level.size() <= numberOfColors) {
			std::sort(level.begin(), level.end(), [](Node *a, Node *b) {
				return a->totalPixels() < b->totalPixels();
			});
		}
		for (auto node: level) {
			if (accurate && (m_leafs - numberOfColors < 8)) {
				m_leafs -= node->reduceLeafs(m_leafs - numberOfColors, *this);
			} else {
				m_leafs -= node->removeLeafs(*this);
			}
			if (m_leafs <= numberOfColors) {
				int levelIndex = i;
				for (; i < static_cast<int>(maxDepth - 1); ++i) {
					m_levels[i].clear();
				}
				rebuildLevel(levelIndex);
				return;
			}
		}
	}
	if (m_leafs - numberOfColors < 8) {
		m_leafs -= m_root.reduceLeafs(m_leafs - numberOfColors, *this);
	} else {
		m_leafs -= m_root.removeLeafs(*this);
	}
	for (int i = 0; i < static_cast<int>(maxDepth - 1); ++i) {
		m_levels[i].clear();
	}
}
size_t OctreeColorQuantization::size() const {
	return m_leafs;
}
}
