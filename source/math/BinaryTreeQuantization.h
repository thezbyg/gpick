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

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <vector>
namespace math {
template<typename T>
struct BinaryTreeQuantization {
	static constexpr uint8_t children = 2;
	static constexpr uint8_t maxDepth = 8;
	static constexpr size_t maxNodesPerLevel = 512;
	struct Node;
	using Allocator = std::pmr::polymorphic_allocator<Node>;
	struct Node {
		Node() noexcept;
		Node(const Node &node, uint8_t depth, BinaryTreeQuantization &btq);
		Node &operator=(const Node &node);
		void add(const T &value, uint8_t depth, BinaryTreeQuantization &btq);
		void add(const T &value, size_t count, uint8_t depth, BinaryTreeQuantization &btq);
		T find(const T &value, uint8_t depth) const;
		void clear();
		size_t removeLeafs(BinaryTreeQuantization &btq);
		size_t leafs() const;
		bool isLeaf() const;
		size_t totalCount() const;
		T totalSum() const;
		template<typename Callback>
		void visit(Callback &&callback) const {
			if (isLeaf()) {
				callback(m_sum, m_count);
				return;
			}
			for (uint8_t i = 0; i < children; i++) {
				if (m_children[i])
					m_children[i]->visit(callback);
			}
		}
	private:
		Node *m_children[children];
		size_t m_count;
		T m_sum;
		friend struct BinaryTreeQuantization;
	};
	BinaryTreeQuantization();
	BinaryTreeQuantization(const BinaryTreeQuantization &btq);
	void add(const T &value);
	void add(const T &value, size_t count);
	T find(const T &value) const;
	void clear();
	void reduce(size_t count);
	void reduceByMinDistance(const T &minDifference);
	size_t size() const;
	template<typename Callback>
	void visit(Callback &&callback) {
		m_root.visit(std::forward<Callback>(callback));
	}
private:
	size_t m_leafs;
	std::array<std::vector<Node *>, maxDepth - 1> m_levels;
	std::vector<Node *> m_freeNodes;
	std::vector<uint8_t> m_memory;
	std::pmr::monotonic_buffer_resource m_bufferResource;
	Allocator m_allocator;
	Node m_root;
	void rebuildLevel(uint8_t level);
	friend struct Node;
};
}
