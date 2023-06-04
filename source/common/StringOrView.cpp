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

#include "StringOrView.h"
#include <new>
#include <utility>
namespace common {
StringOrView::StringOrView():
	m_type(Type::view) {
	new (&m_view) std::string_view();
}
StringOrView::StringOrView(const char *value) noexcept:
	m_type(Type::view) {
	new (&m_view) std::string_view(value);
}
StringOrView::StringOrView(std::string_view value) noexcept:
	m_type(Type::view) {
	new (&m_view) std::string_view(value);
}
StringOrView::StringOrView(std::string &&value) noexcept:
	m_type(Type::string) {
	new (&m_string) std::string(std::move(value));
}
StringOrView::StringOrView(StringOrView &&value) noexcept:
	m_type(value.m_type) {
	switch (m_type) {
	case Type::view:
		new (&m_view) std::string_view(value.m_view);
		break;
	case Type::string:
		new (&m_string) std::string(std::move(value.m_string));
		break;
	}
}
StringOrView::~StringOrView() {
	switch (m_type) {
	case Type::view:
		m_view.~basic_string_view();
		break;
	case Type::string:
		m_string.~basic_string();
		break;
	}
}
std::size_t StringOrView::length() const {
	switch (m_type) {
	case Type::view:
		return m_view.length();
	case Type::string:
		return m_string.length();
	}
	return 0;
}
void StringOrView::reset() {
	switch (m_type) {
	case Type::view:
		m_view.~basic_string_view();
		break;
	case Type::string:
		m_string.~basic_string();
		break;
	}
	m_type = Type::view;
	new (&m_view) std::string_view();
}
std::string_view StringOrView::view() const {
	switch (m_type) {
	case Type::view:
		return m_view;
	case Type::string:
		return m_string;
	}
	return std::string_view();
}
}
