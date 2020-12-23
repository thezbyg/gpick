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

#ifndef GPICK_COMMON_RESULT_H_
#define GPICK_COMMON_RESULT_H_
#include <utility>
namespace common {
template<typename ValueT, typename ErrorT> struct Result {
	Result():
		m_status(false) {
	}
	Result(const ErrorT &error):
		m_error(error),
		m_status(false) {
	}
	Result(const ValueT &value):
		m_value(value),
		m_status(true) {
	}
	Result(ErrorT &&error):
		m_error(std::move(error)),
		m_status(false) {
	}
	Result(ValueT &&value):
		m_value(std::move(value)),
		m_status(true) {
	}
	Result(const Result &) = delete;
	Result(Result &&result):
		m_value(std::move(result.m_value)),
		m_error(std::move(result.m_error)),
		m_status(result.m_status) {
	}
	Result &operator=(const Result &) = delete;
	Result &operator=(Result &&result) {
		m_status = result.m_status;
		m_value = std::move(result.m_value);
		m_error = std::move(result.m_error);
		return *this;
	}
	~Result() {
	}
	operator bool() const {
		return m_status;
	}
	const ValueT &value() const {
		return m_value;
	}
	ValueT &value() {
		return m_value;
	}
	const ErrorT &error() const {
		return m_error;
	}
	ErrorT &error() {
		return m_error;
	}
private:
	ValueT m_value;
	ErrorT m_error;
	bool m_status;
};
template<typename BothT> struct Result<BothT, BothT> {
	Result():
		m_status(false) {
	}
	Result(bool status, const BothT &value):
		m_value(value),
		m_status(status) {
	}
	Result(bool status, BothT &&value):
		m_value(std::move(value)),
		m_status(status) {
	}
	Result(const Result &) = delete;
	Result(Result &&result):
		m_value(std::move(result.m_value)),
		m_status(result.m_status) {
	}
	Result &operator=(const Result &) = delete;
	Result &operator=(Result &&result) {
		m_value = std::move(result.m_value);
		return *this;
	}
	~Result() {
	}
	operator bool() const {
		return m_status;
	}
	const BothT &value() const {
		return m_value;
	}
	BothT &value() {
		return m_value;
	}
	const BothT &error() const {
		return m_value;
	}
	BothT &error() {
		return m_value;
	}
private:
	BothT m_value;
	bool m_status;
};
template<typename ErrorT> struct Result<void, ErrorT> {
	Result():
		m_status(true) {
	}
	Result(const ErrorT &error):
		m_error(error),
		m_status(false) {
	}
	Result(ErrorT &&error):
		m_error(std::move(error)),
		m_status(false) {
	}
	Result(const Result &) = delete;
	Result(Result &&result):
		m_error(std::move(result.m_error)),
		m_status(result.m_status) {
	}
	Result &operator=(const Result &) = delete;
	Result &operator=(Result &&result) {
		m_error = std::move(result.m_error);
		return *this;
	}
	~Result() {
	}
	operator bool() const {
		return m_status;
	}
	const ErrorT &error() const {
		return m_error;
	}
	ErrorT &error() {
		return m_error;
	}
private:
	ErrorT m_error;
	bool m_status;
};
template<typename ValueT> struct Result<ValueT, void> {
	Result():
		m_status(false) {
	}
	Result(const ValueT &value):
		m_value(value),
		m_status(true) {
	}
	Result(ValueT &&value):
		m_value(std::move(value)),
		m_status(true) {
	}
	Result(const Result &) = delete;
	Result(Result &&result):
		m_value(std::move(result.m_value)),
		m_status(result.m_status) {
	}
	Result &operator=(const Result &) = delete;
	Result &operator=(Result &&result) {
		m_value = std::move(result.m_value);
		return *this;
	}
	~Result() {
	}
	operator bool() const {
		return m_status;
	}
	const ValueT &value() const {
		return m_value;
	}
	ValueT &value() {
		return m_value;
	}
private:
	ValueT m_value;
	bool m_status;
};
template<typename ErrorT> using ResultVoid = struct Result<void, ErrorT>;
}
#endif /* GPICK_COMMON_RESULT_H_ */
