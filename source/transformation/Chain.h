/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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
#include <vector>
#include <memory>
struct Color;
namespace transformation {
struct Transformation;
struct Chain {
	Chain();
	Chain &operator=(const Chain &chain);
	Chain &operator=(Chain &&chain);
	operator bool() const;
	Color apply(Color input);
	void add(std::unique_ptr<Transformation> &&transformation);
	void remove(const Transformation &transformation);
	void move(const Transformation &transformation, size_t newIndex);
	void enable(bool enabled);
	bool enabled() const;
	struct Iterator {
		bool operator!=(const Iterator &iterator) const;
		Iterator &operator++();
		Transformation &operator*();
	private:
		Iterator(Chain &chain, size_t index);
		Chain &m_chain;
		size_t m_index;
		friend struct Chain;
	};
	Iterator begin();
	Iterator end();
private:
	std::vector<std::unique_ptr<Transformation>> m_transformations;
	bool m_enabled;
	friend struct Iterator;
};
}
