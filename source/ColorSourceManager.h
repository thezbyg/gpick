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
#include "dynv/MapFwd.h"
#include "common/Bitmask.h"
#include <unordered_set>
#include <string>
#include <string_view>
#include <memory>
#include <boost/unordered_set.hpp>
struct GlobalState;
struct IColorSource;
enum struct RegistrationFlags: uint32_t {
	none = 0,
	singleInstanceOnly = 1,
	needsViewport = 2,
};
ENABLE_BITMASK_OPERATORS(RegistrationFlags);
const RegistrationFlags test = ~RegistrationFlags::singleInstanceOnly;
struct ColorSourceManager {
	using Build = std::unique_ptr<IColorSource> (*)(GlobalState &gs, const dynv::Ref &options);
	struct Registration {
		Registration(std::string_view name, std::string_view label, RegistrationFlags flags, int defaultAccelerator, Build build);
		const std::string name, label;
		const bool singleInstanceOnly, needsViewport;
		const int defaultAccelerator;
		operator bool() const;
		const Build build;
		template<typename T>
		std::unique_ptr<T> make(GlobalState &gs, const dynv::Ref &options) const {
			return std::unique_ptr<T>(static_cast<T *>(build(gs, options).release()));
		}
	private:
		friend struct ColorSourceManager;
	};
	void add(Registration &&registration);
	void add(std::string_view name, std::string_view label, RegistrationFlags flags, int defaultAccelerator, Build build);
	bool has(std::string_view name) const;
	const Registration &operator[](std::string_view name) const;
	template<typename VisitorT>
	void visit(VisitorT visitor) {
		for (const auto &registration: m_registrations) {
			visitor(registration);
		}
	}
private:
	struct Hash {
		bool operator()(const Registration &lhs, const Registration &rhs) const {
			return lhs.name == rhs.name;
		}
		std::size_t operator()(const Registration &registration) const {
			return std::hash<std::string>()(registration.name);
		}
	};
	boost::unordered_set<Registration, Hash, Hash> m_registrations;
};
