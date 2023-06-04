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

#include "BinaryTreeQuantization.h"
#include "Algorithms.h"
#include <algorithm>
#include <cstring>
#include <new>
namespace math {
template<typename T>
using Node = typename BinaryTreeQuantization<T>::Node;
template<typename T, typename... Args>
Node<T> *newNode(typename BinaryTreeQuantization<T>::Allocator &allocator, std::vector<Node<T> *> &freeNodes, Args &&...args) {
	Node<T> *node;
	if (freeNodes.size() > 0) {
		node = freeNodes.back();
		freeNodes.pop_back();
	} else {
		node = allocator.allocate(1);
	}
	return new (node) Node<T>(std::forward<Args>(args)...);
}
template<size_t MaxNodesPerLevel>
constexpr size_t maxTotalNodes(size_t value, uint8_t depth) {
	if (depth == 0)
		return std::min(value, MaxNodesPerLevel);
	return std::min(value, MaxNodesPerLevel) + maxTotalNodes<MaxNodesPerLevel>(value * 2, --depth);
}
template<typename T>
BinaryTreeQuantization<T>::Node::Node() noexcept:
	m_children { nullptr },
	m_count(0),
	m_sum { 0 } {
}
template<typename T>
BinaryTreeQuantization<T>::Node::Node(const Node &node, uint8_t depth, BinaryTreeQuantization &btq):
	m_children { nullptr },
	m_count(node.m_count),
	m_sum { node.m_sum } {
	if (m_count > 0)
		return;
	for (uint8_t i = 0; i < children; i++) {
		if (node.m_children[i]) {
			m_children[i] = newNode<T>(btq.m_allocator, btq.m_freeNodes, *node.m_children[i], depth + 1, btq);
			if (depth < maxDepth - 1)
				btq.m_levels[depth].push_back(m_children[i]);
		} else {
			m_children[i] = nullptr;
		}
	}
}
template<typename T>
void BinaryTreeQuantization<T>::Node::clear() {
	m_count = 0;
	m_sum = { 0 };
	std::memset(m_children, 0, sizeof(m_children));
}
template<typename T>
size_t BinaryTreeQuantization<T>::Node::leafs() const {
	size_t result = isLeaf() ? 1 : 0;
	for (uint8_t i = 0; i < children; i++) {
		if (m_children[i])
			result += m_children[i]->leafs();
	}
	return result;
}
template<typename T>
size_t BinaryTreeQuantization<T>::Node::removeLeafs(BinaryTreeQuantization &btq) {
	bool wasLeaf = isLeaf();
	uint8_t result = 0;
	for (uint8_t i = 0; i < children; i++) {
		if (!m_children[i])
			continue;
		result += m_children[i]->removeLeafs(btq);
		auto &node = *m_children[i];
		btq.m_freeNodes.push_back(m_children[i]);
		m_children[i] = nullptr;
		m_count += node.totalCount();
		m_sum += node.totalSum();
		++result;
	}
	if (!wasLeaf && isLeaf())
		--result;
	return result;
}
template<typename T>
uint8_t toIndex(const T &value, uint8_t depth) {
	return (std::min(255, static_cast<int>(256 * value)) >> (7 - depth)) & 1;
}
template<typename T>
void BinaryTreeQuantization<T>::Node::add(const T &value, uint8_t depth, BinaryTreeQuantization &btq) {
	if (btq.m_leafs + 1 >= maxNodesPerLevel)
		btq.reduce(maxNodesPerLevel / 2);
	if (depth == maxDepth || isLeaf()) {
		if (m_count == 0)
			btq.m_leafs++;
		m_count++;
		m_sum += value;
		return;
	}
	uint8_t i = toIndex(value, depth);
	if (!m_children[i]) {
		m_children[i] = newNode<T>(btq.m_allocator, btq.m_freeNodes);
		if (depth < maxDepth - 1)
			btq.m_levels[depth].push_back(m_children[i]);
	}
	m_children[i]->add(value, depth + 1, btq);
}
template<typename T>
void BinaryTreeQuantization<T>::Node::add(const T &value, size_t count, uint8_t depth, BinaryTreeQuantization &btq) {
	if (btq.m_leafs + 1 >= maxNodesPerLevel)
		btq.reduce(maxNodesPerLevel / 2);
	if (depth == maxDepth || isLeaf()) {
		if (m_count == 0)
			btq.m_leafs++;
		m_count += count;
		m_sum += value * count;
		return;
	}
	uint8_t i = toIndex<T>(value, depth);
	if (!m_children[i]) {
		m_children[i] = newNode<T>(btq.m_allocator, btq.m_freeNodes);
		if (depth < maxDepth - 1)
			btq.m_levels[depth].push_back(m_children[i]);
	}
	m_children[i]->add(value, count, depth + 1, btq);
}
template<typename T>
T BinaryTreeQuantization<T>::Node::find(const T &value, uint8_t depth) const {
	if (depth == maxDepth || isLeaf())
		return m_sum / m_count;
	uint8_t i = toIndex<T>(value, depth);
	if (m_children[i] != nullptr)
		return m_children[i]->find(value, depth + 1);
	else
		return totalSum() / totalCount();
}
template<typename T>
T BinaryTreeQuantization<T>::Node::totalSum() const {
	T result = m_sum;
	for (uint8_t i = 0; i < children; i++) {
		if (m_children[i]) {
			result += m_children[i]->totalSum();
		}
	}
	return result;
}
template<typename T>
bool BinaryTreeQuantization<T>::Node::isLeaf() const {
	return m_count > 0;
}
template<typename T>
size_t BinaryTreeQuantization<T>::Node::totalCount() const {
	size_t result = m_count;
	for (uint8_t i = 0; i < children; i++) {
		if (!m_children[i])
			continue;
		result += m_children[i]->totalCount();
	}
	return result;
}
template<typename T>
BinaryTreeQuantization<T>::BinaryTreeQuantization():
	m_leafs(0),
	m_memory(maxTotalNodes<maxNodesPerLevel>(1, maxDepth) * sizeof(Node)),
	m_bufferResource(m_memory.data(), m_memory.size(), std::pmr::null_memory_resource()),
	m_allocator(&m_bufferResource) {
}
template<typename T>
BinaryTreeQuantization<T>::BinaryTreeQuantization(const BinaryTreeQuantization &btq):
	m_leafs(btq.m_leafs),
	m_memory(maxTotalNodes<maxNodesPerLevel>(1, maxDepth) * sizeof(Node)),
	m_bufferResource(m_memory.data(), m_memory.size(), std::pmr::null_memory_resource()),
	m_allocator(&m_bufferResource),
	m_root(btq.m_root, 0, *this) {
}
template<typename T>
void BinaryTreeQuantization<T>::add(const T &value) {
	m_root.add(value, 0, *this);
}
template<typename T>
void BinaryTreeQuantization<T>::add(const T &value, size_t count) {
	m_root.add(value, count, 0, *this);
}
template<typename T>
T BinaryTreeQuantization<T>::find(const T &value) const {
	return m_root.find(value, 0);
}
template<typename T>
void BinaryTreeQuantization<T>::clear() {
	m_root.clear();
	m_bufferResource.release();
	for (auto &level: m_levels)
		level.clear();
	m_freeNodes.clear();
	m_leafs = 0;
}
template<typename T>
void BinaryTreeQuantization<T>::rebuildLevel(uint8_t level) {
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
template<typename T>
void BinaryTreeQuantization<T>::reduce(size_t count) {
	if (m_leafs <= count)
		return;
	for (int i = static_cast<int>(maxDepth - 2); i >= 0; --i) {
		std::vector<Node *> &level = m_levels[i];
		if (level.size() <= count) {
			std::sort(level.begin(), level.end(), [](Node *a, Node *b) {
				return a->totalCount() < b->totalCount();
			});
		}
		for (auto node: level) {
			m_leafs -= node->removeLeafs(*this);
			if (m_leafs <= count) {
				int levelIndex = i;
				for (; i < static_cast<int>(maxDepth - 1); ++i) {
					m_levels[i].clear();
				}
				rebuildLevel(levelIndex);
				return;
			}
		}
	}
	m_leafs -= m_root.removeLeafs(*this);
	for (int i = 0; i < static_cast<int>(maxDepth - 1); ++i) {
		m_levels[i].clear();
	}
}
template<typename T>
void BinaryTreeQuantization<T>::reduceByMinDistance(const T &minDifference) {
	for (int i = static_cast<int>(maxDepth - 2); i >= 0; --i) {
		std::vector<Node *> &level = m_levels[i];
		for (auto *node: level) {
			if (node->m_children[0] == nullptr || node->m_children[1] == nullptr)
				continue;
			T first = node->m_children[0]->totalSum() / node->m_children[0]->totalCount();
			T second = node->m_children[1]->totalSum() / node->m_children[1]->totalCount();
			if (math::abs(first - second) < minDifference) {
				m_leafs -= node->removeLeafs(*this);
			}
		}
	}
	if (m_root.m_children[0] != nullptr && m_root.m_children[1] != nullptr) {
		T first = m_root.m_children[0]->totalSum() / m_root.m_children[0]->totalCount();
		T second = m_root.m_children[1]->totalSum() / m_root.m_children[1]->totalCount();
		if (math::abs(first - second) < minDifference) {
			m_leafs -= m_root.removeLeafs(*this);
		}
	}
	int levelIndex = 0;
	for (int j = 0; j < static_cast<int>(maxDepth - 1); ++j) {
		m_levels[j].clear();
		rebuildLevel(levelIndex);
	}
}
template<typename T>
size_t BinaryTreeQuantization<T>::size() const {
	return m_leafs;
}
template bool BinaryTreeQuantization<float>::Node::isLeaf() const;
template BinaryTreeQuantization<float>::BinaryTreeQuantization();
template BinaryTreeQuantization<float>::BinaryTreeQuantization(const BinaryTreeQuantization<float> &btq);
template void BinaryTreeQuantization<float>::add(const float &value);
template void BinaryTreeQuantization<float>::add(const float &value, size_t count);
template float BinaryTreeQuantization<float>::find(const float &value) const;
template void BinaryTreeQuantization<float>::clear();
template void BinaryTreeQuantization<float>::reduce(size_t count);
template void BinaryTreeQuantization<float>::reduceByMinDistance(const float &minDifference);
template size_t BinaryTreeQuantization<float>::size() const;
}
