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

#ifndef GPICK_MATH_OCTREE_COLOR_QUANTIZATION_H_
#define GPICK_MATH_OCTREE_COLOR_QUANTIZATION_H_
#include "Color.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <utility>
#include <vector>
namespace math {
struct OctreeColorQuantization {
	static constexpr uint8_t children = 8;
	static constexpr uint8_t maxDepth = 8;
	static constexpr size_t maxNodesPerLevel = 4096;
	struct Node;
	using Allocator = std::pmr::polymorphic_allocator<Node>;
	using Position = std::array<uint8_t, 3>;
	struct Node {
		Node() noexcept;
		Node(const Node &node, uint8_t depth, OctreeColorQuantization &ocq);
		Node &operator=(const Node &node);
		void add(const Color &color, const Position position, uint8_t depth, OctreeColorQuantization &ocq);
		void add(const Color &color, size_t pixels, const Position position, uint8_t depth, OctreeColorQuantization &ocq);
		void clear();
		uint8_t removeLeafs(OctreeColorQuantization &ocq);
		uint8_t reduceLeafs(uint8_t reduceBy, OctreeColorQuantization &ocq);
		size_t leafs() const;
		bool isLeaf() const;
		size_t totalPixels() const;
		template<typename Callback>
		void visit(Callback &&callback) const {
			if (isLeaf()) {
				callback(m_colorSum, m_pixels);
				return;
			}
			for (uint8_t i = 0; i < children; i++) {
				if (m_children[i])
					m_children[i]->visit(callback);
			}
		}
	private:
		Node *m_children[children];
		size_t m_pixels;
		float m_colorSum[3];
		friend struct OctreeColorQuantization;
	};
	OctreeColorQuantization();
	OctreeColorQuantization(const OctreeColorQuantization &ocq);
	void add(const Color &color, const Position position);
	void add(const Color &color, size_t pixels, const Position position);
	void clear();
	void reduce(size_t numberOfColors, bool accurate = true);
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
#endif /* GPICK_MATH_OCTREE_COLOR_QUANTIZATION_H_ */
