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

#include "ColorSourceManager.h"
namespace {
struct Match {
	bool operator()(const std::string_view &lhs, const ColorSourceManager::Registration &rhs) const {
		return lhs == rhs.name;
	}
	std::size_t operator()(const std::string_view &name) const {
		return std::hash<std::string_view>()(name);
	}
};
}
ColorSourceManager::Registration::Registration(std::string_view name, std::string_view label, RegistrationFlags flags, int defaultAccelerator, Build build):
	name(name),
	label(label),
	singleInstanceOnly((flags & RegistrationFlags::singleInstanceOnly) != RegistrationFlags::none),
	needsViewport((flags & RegistrationFlags::needsViewport) != RegistrationFlags::none),
	defaultAccelerator(defaultAccelerator),
	build(build) {
}
ColorSourceManager::Registration::operator bool() const {
	return build != nullptr;
}
void ColorSourceManager::add(Registration &&registration) {
	m_registrations.emplace(std::move(registration));
}
void ColorSourceManager::add(std::string_view name, std::string_view label, RegistrationFlags flags, int defaultAccelerator, Build build) {
	m_registrations.emplace(name, label, flags, defaultAccelerator, build);
}
bool ColorSourceManager::has(std::string_view name) const {
	return m_registrations.find(name, Match {}, Match {}) != m_registrations.end();
}
const ColorSourceManager::Registration &ColorSourceManager::operator[](std::string_view name) const {
	auto i = m_registrations.find(name, Match {}, Match {});
	if (i != m_registrations.end())
		return *i;
	static Registration nullRegistration("", "", RegistrationFlags::none, 0, nullptr);
	return nullRegistration;
}
