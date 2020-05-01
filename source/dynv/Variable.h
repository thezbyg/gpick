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

#ifndef GPICK_DYNV_VARIABLE_H_
#define GPICK_DYNV_VARIABLE_H_
#include "MapFwd.h"
#include "Color.h"
#include "common/Ref.h"
#include <string>
#include <vector>
#include <cstdint>
#include <boost/variant.hpp>
namespace dynv {
struct Map;
struct Variable {
	using Data = boost::variant<bool, float, int32_t, Color, std::string, Ref, std::vector<bool>, std::vector<float>, std::vector<int32_t>, std::vector<Color>, std::vector<std::string>, std::vector<Ref>>;
	Variable(const std::string &name, bool value);
	Variable(const std::string &name, float value);
	Variable(const std::string &name, int32_t value);
	Variable(const std::string &name, const Color &value);
	Variable(const std::string &name, const std::string &value);
	Variable(const std::string &name, const char *value);
	Variable(const std::string &name, const Ref &value);
	Variable(const std::string &name, const std::vector<bool> &value);
	Variable(const std::string &name, const std::vector<float> &value);
	Variable(const std::string &name, const std::vector<int32_t> &value);
	Variable(const std::string &name, const std::vector<Color> &value);
	Variable(const std::string &name, const std::vector<std::string> &value);
	Variable(const std::string &name, const std::vector<const char *> &value);
	Variable(const std::string &name, const std::vector<Ref> &value);
	void assign(bool value);
	void assign(float value);
	void assign(int32_t value);
	void assign(const Color &value);
	void assign(const std::string &value);
	void assign(const char *value);
	void assign(const Ref &value);
	template<typename T>
	void assign(const std::vector<T> &value);
	~Variable();
	const std::string &name() const;
	const Data &data() const;
	Data &data();
private:
	std::string m_name;
	Data m_data;
};
}
#endif /* GPICK_DYNV_VARIABLE_H_ */
