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
#include <type_traits>
namespace common {
struct NullRef {
	explicit constexpr NullRef(int) {
	}
};
void validateRefCounterDestruction(void *thisPointer, uint32_t referenceCounter);
template<typename T>
struct Ref {
	struct Counter {
		Counter():
			m_referenceCounter(1) {
		}
		virtual ~Counter() {
			validateRefCounterDestruction(this, m_referenceCounter);
		}
		Counter(const Counter &) = delete;
		Counter &operator=(const Counter &) = delete;
		Counter(Counter &&counter):
			m_referenceCounter(counter.m_referenceCounter) {
			counter.m_referenceCounter = 1;
		}
		T *reference() {
			m_referenceCounter++;
			return static_cast<T *>(this);
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
	Ref() noexcept:
		m_value(nullptr) {
	}
	Ref(const NullRef &) noexcept:
		m_value(nullptr) {
	}
	explicit Ref(T *value) noexcept:
		m_value(value) {
	}
	Ref(const Ref<T> &reference) noexcept:
		m_value(reference.m_value ? reference.m_value->reference() : nullptr) {
	}
	template<typename OtherT, std::enable_if_t<std::is_base_of_v<T, OtherT> && !std::is_same_v<T, OtherT>, int> = 0>
	Ref(const Ref<OtherT> &reference) noexcept:
		m_value(reference.m_value ? reference.m_value->reference() : nullptr) {
	}
	Ref(Ref<T> &&reference) noexcept:
		m_value(reference.m_value) {
		reference.m_value = nullptr;
	}
	Ref &operator=(const Ref<T> &reference) {
		if (m_value)
			m_value->release();
		m_value = reference.m_value ? reference.m_value->reference() : nullptr;
		return *this;
	}
	template<typename OtherT, std::enable_if_t<std::is_base_of_v<T, OtherT> && !std::is_same_v<T, OtherT>, int> = 0>
	Ref &operator=(const Ref<OtherT> &reference) {
		if (m_value)
			m_value->release();
		m_value = reference.m_value ? reference.m_value->reference() : nullptr;
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
	bool operator!=(const Ref<T> &reference) const {
		return m_value != reference.m_value;
	}
	template<typename OtherT, std::enable_if_t<std::is_base_of_v<T, OtherT> && !std::is_same_v<T, OtherT>, int> = 0>
	bool operator==(const Ref<OtherT> &reference) const {
		return m_value == reference.m_value;
	}
	template<typename OtherT, std::enable_if_t<std::is_base_of_v<T, OtherT> && !std::is_same_v<T, OtherT>, int> = 0>
	bool operator!=(const Ref<OtherT> &reference) const {
		return m_value != reference.m_value;
	}
	bool operator==(const T *value) const {
		return m_value == value;
	}
	bool operator!=(const T *value) const {
		return m_value != value;
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
	T *pointer() {
		return m_value;
	}
	const T *pointer() const {
		return m_value;
	}
	[[nodiscard]] static Ref wrap(T *value) {
		return Ref<T>(value ? value->reference() : nullptr);
	}
	[[nodiscard]] T *unwrap() {
		T *result = m_value;
		m_value = nullptr;
		return result;
	}
private:
	T *m_value;
	template<typename OtherT>
	friend struct Ref;
};
inline constexpr NullRef nullRef { 0 };
template<typename T>
Ref<T> wrapInRef(T *value) {
	return Ref<T>::wrap(value);
}
}
#endif /* GPICK_COMMON_REF_H_ */
