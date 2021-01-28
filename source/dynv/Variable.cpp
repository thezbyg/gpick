/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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

#include "Variable.h"
#include "Map.h"
namespace dynv {
Variable::Variable(const std::string &name, bool value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, float value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, int32_t value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const Color &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const std::string &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const char *value):
	m_name(name),
	m_data(std::string(value)) {
}
Variable::Variable(const std::string &name, const Ref &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const std::vector<bool> &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const std::vector<float> &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const std::vector<int32_t> &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const std::vector<Color> &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const std::vector<std::string> &value):
	m_name(name),
	m_data(value) {
}
Variable::Variable(const std::string &name, const std::vector<const char *> &value):
	m_name(name),
	m_data(std::vector<std::string>(value.begin(), value.end())) {
}
Variable::Variable(const std::string &name, const std::vector<Ref> &value):
	m_name(name),
	m_data(value) {
}
void Variable::assign(bool value) {
	m_data = value;
}
void Variable::assign(float value) {
	m_data = value;
}
void Variable::assign(int32_t value) {
	m_data = value;
}
void Variable::assign(const Color &value) {
	m_data = value;
}
void Variable::assign(const std::string &value) {
	m_data = value;
}
void Variable::assign(const char *value) {
	m_data = std::string(value);
}
void Variable::assign(const Ref &value) {
	m_data = value;
}
template<>
void Variable::assign(const std::vector<bool> &value) {
	m_data = std::move(value);
}
template<>
void Variable::assign(const std::vector<int32_t> &value) {
	m_data = std::move(value);
}
template<>
void Variable::assign(const std::vector<float> &value) {
	m_data = std::move(value);
}
template<>
void Variable::assign(const std::vector<Color> &value) {
	m_data = std::move(value);
}
template<>
void Variable::assign(const std::vector<const char *> &value) {
	m_data = std::vector<std::string>(value.begin(), value.end());
}
template<>
void Variable::assign(const std::vector<std::string> &value) {
	m_data = value;
}
template<>
void Variable::assign(const std::vector<Ref> &value) {
	m_data = std::move(value);
}
const std::string &Variable::name() const {
	return m_name;
}
const Variable::Data &Variable::data() const {
	return m_data;
}
Variable::Data &Variable::data() {
	return m_data;
}
Variable::~Variable() {
}
}
