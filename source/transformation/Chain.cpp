/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

#include "Chain.h"
#include "Transformation.h"
#include "Color.h"
#include "dynv/Map.h"
namespace transformation {
Chain::Chain():
	m_enabled(true) {
}
Chain &Chain::operator=(const Chain &chain) {
	m_enabled = chain.m_enabled;
	m_transformations.reserve(chain.m_transformations.size());
	for (const auto &transformation: chain.m_transformations) {
		m_transformations.emplace_back(std::move(transformation->copy()));
	}
	return *this;
}
Chain &Chain::operator=(Chain &&chain) {
	m_enabled = chain.m_enabled;
	m_transformations = std::move(chain.m_transformations);
	chain.m_enabled = true;
	return *this;
}
Chain::operator bool() const {
	if (!m_enabled)
		return false;
	return !m_transformations.empty();
}
Color Chain::apply(Color input) {
	if (!m_enabled)
		return input;
	auto result = input;
	for (auto &transformation: m_transformations) {
		result = transformation->apply(result);
	}
	return result;
}
void Chain::add(std::unique_ptr<Transformation> &&transformation) {
	m_transformations.emplace_back(std::move(transformation));
}
void Chain::remove(const Transformation &transformation) {
	auto i = std::find_if(m_transformations.begin(), m_transformations.end(), [&transformation](std::unique_ptr<Transformation> &value) {
		return value.get() == &transformation;
	});
	if (i == m_transformations.end())
		return;
	m_transformations.erase(i);
}
void Chain::move(const Transformation &transformation, size_t newIndex) {
	auto i = std::find_if(m_transformations.begin(), m_transformations.end(), [&transformation](std::unique_ptr<Transformation> &value) {
		return value.get() == &transformation;
	});
	if (i == m_transformations.end())
		return;
	auto currentIndex = static_cast<size_t>(std::distance(m_transformations.begin(), i));
	if (currentIndex == newIndex)
		return;
	auto value = std::move(*i);
	m_transformations.erase(i);
	m_transformations.emplace(m_transformations.begin() + newIndex - (newIndex > currentIndex ? 1 : 0), std::move(value));
}
void Chain::enable(bool enabled) {
	m_enabled = enabled;
}
bool Chain::enabled() const {
	return m_enabled;
}
bool Chain::Iterator::operator!=(const Iterator &iterator) const {
	return m_index != iterator.m_index;
}
Chain::Iterator &Chain::Iterator::operator++() {
	++m_index;
	return *this;
}
Transformation &Chain::Iterator::operator*() {
	return *m_chain.m_transformations[m_index];
}
Chain::Iterator::Iterator(Chain &chain, size_t index):
	m_chain(chain),
	m_index(index) {
}
Chain::Iterator Chain::begin() {
	return Iterator(*this, 0);
}
Chain::Iterator Chain::end() {
	return Iterator(*this, m_transformations.size());
}
}
