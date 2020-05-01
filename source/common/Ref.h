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

#ifndef GPICK_COMMON_REF_H_
#define GPICK_COMMON_REF_H_
#include <cstdint>
#include <iostream>
namespace common {
template<typename T>
struct Ref {
	struct Counter {
		Counter():
			m_referenceCounter(1) {
		}
		virtual ~Counter() {
			if (m_referenceCounter > 1) {
				std::cerr << "Referenced value destroyed [address: " << this << ", reference count: " << m_referenceCounter << "]\n";
			}
		}
		T *reference() {
			m_referenceCounter++;
			return reinterpret_cast<T *>(this);
		}
		bool release() {
			if (m_referenceCounter > 1) {
				m_referenceCounter--;
				return false;
			} else {
				delete this;
				return true;
			}
		}
		uint32_t references() const {
			return m_referenceCounter;
		}
	private:
		uint32_t m_referenceCounter;
	};
	Ref():
		m_value(nullptr) {
	}
	Ref(T *value):
		m_value(value) {
	}
	Ref(const Ref<T> &reference):
		m_value(reference.m_value->reference()) {
	}
	Ref(Ref<T> &&reference):
		m_value(reference.m_value) {
		reference.m_value = nullptr;
	}
	Ref &operator=(const Ref<T> &reference) {
		if (m_value)
			m_value->release();
		m_value = reference.m_value->reference();
		return *this;
	}
	~Ref() {
		if (m_value)
			m_value->release();
	}
	bool release() {
		bool result = true;
		if (m_value)
			result = m_value->release();
		m_value = nullptr;
		return result;
	}
	explicit operator bool() const {
		return m_value != nullptr;
	}
	bool operator==(const Ref<T> &reference) const {
		return m_value == reference.m_value;
	}
	T &operator*() {
		return *m_value;
	}
	const T &operator*() const {
		return *m_value;
	}
	T *operator->() {
		return m_value;
	}
	const T *operator->() const {
		return m_value;
	}
	uint32_t references() const {
		return m_value ? m_value->references() : 0;
	}
private:
	T *m_value;
};
}
#endif /* GPICK_COMMON_REF_H_ */
