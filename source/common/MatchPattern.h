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

#pragma once
#include <array>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
namespace common {
namespace detail {
template<typename CharT>
struct CharOp {
	CharOp(CharT value):
		m_value(value) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		if (position >= value.length())
			return false;
		if (value[position] != m_value)
			return false;
		position++;
		return true;
	}
private:
	CharT m_value;
};
template<typename CharT, std::size_t Count>
struct CharsOp {
	template<std::size_t... Index>
	CharsOp(const CharT (&values)[Count], std::index_sequence<Index...>):
		m_values { values[Index]... } {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		if (position >= value.length())
			return false;
		bool matched = false;
		for (auto ch: m_values) {
			if (value[position] == ch) {
				matched = true;
				break;
			}
		}
		if (!matched)
			return false;
		position++;
		return true;
	}
private:
	std::array<CharT, Count> m_values;
};
template<typename StringT>
struct StringOp {
	StringOp(StringT value):
		m_value(value) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		auto length = m_value.length();
		if (position + length > value.length())
			return false;
		for (std::size_t i = 0; i < length; i++) {
			if (value[position + i] != m_value[i])
				return false;
		}
		position += length;
		return true;
	}
private:
	StringT m_value;
};
template<typename StartOpT, typename MatchOpT>
struct StartOp {
	StartOp(StartOpT startOp, MatchOpT matchOp):
		m_startOp(startOp),
		m_matchOp(matchOp) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		for (;;) {
			if (position >= value.length())
				return false;
			std::size_t savedPosition = position;
			if (apply(value, position, m_startOp)) {
				if (apply(value, position, m_matchOp)) {
					return true;
				}
			}
			position = savedPosition + 1;
		}
	}
private:
	StartOpT m_startOp;
	MatchOpT m_matchOp;
};
template<typename StartOpT>
struct StartOp<StartOpT, void> {
	StartOp(StartOpT startOp):
		m_startOp(startOp) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		for (;;) {
			if (position >= value.length())
				return false;
			std::size_t savedPosition = position;
			if (apply(value, position, m_startOp)) {
				return true;
			}
			position = savedPosition + 1;
		}
	}
private:
	StartOpT m_startOp;
};
template<typename T, typename OpT>
bool apply(T value, std::size_t &position, OpT op) {
	if constexpr (std::is_same_v<OpT, char> || std::is_same_v<OpT, wchar_t>) {
		return CharOp(op)(value, position);
	} else if constexpr (std::is_same_v<OpT, std::basic_string_view<char>> || std::is_same_v<OpT, std::basic_string<char>> || std::is_same_v<OpT, std::basic_string_view<wchar_t>> || std::is_same_v<OpT, std::basic_string<wchar_t>>) {
		return StringOp(op)(value, position);
	} else if constexpr (std::is_same_v<OpT, const char *>) {
		return StringOp(std::string_view(op))(value, position);
	} else if constexpr (std::is_same_v<OpT, const wchar_t *>) {
		return StringOp(std::wstring_view(op))(value, position);
	} else {
		return op(value, position);
	}
}
template<typename T, typename Tuple, std::size_t... Is>
bool applyAnd(T value, std::size_t &position, Tuple &tuple, std::index_sequence<Is...>) {
	return (apply(value, position, std::get<Is>(tuple)) && ...);
}
template<typename T, typename Tuple, std::size_t... Is>
bool applyOr(T value, std::size_t &position, Tuple &tuple, std::index_sequence<Is...>) {
	return (apply(value, position, std::get<Is>(tuple)) || ...);
}
template<typename T, typename Tuple, std::size_t... Is>
bool applyByIndex(std::size_t index, T value, std::size_t &position, Tuple &tuple, std::index_sequence<Is...>) {
	return ((Is == index && apply(value, position, std::get<Is>(tuple))) || ...);
}
template<typename CharT>
struct CharRangeOp {
	CharRangeOp(CharT from, CharT to):
		m_from(from),
		m_to(to) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		if (position >= value.length())
			return false;
		if (!(value[position] >= m_from && value[position] <= m_to))
			return false;
		position++;
		return true;
	}
private:
	CharT m_from, m_to;
};
struct WhitespaceOp {
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		if (position >= value.length())
			return false;
		auto ch = value[position];
		if (!(ch == ' ' || ch == '\t'))
			return false;
		position++;
		return true;
	}
};
struct DecOp {
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		if (position >= value.length())
			return false;
		auto ch = value[position];
		if (!(ch >= '0' && ch <= '9'))
			return false;
		position++;
		return true;
	}
};
struct HexOp {
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		if (position >= value.length())
			return false;
		auto ch = value[position];
		if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')))
			return false;
		position++;
		return true;
	}
};
struct End {
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		return value.length() == position;
	}
};
template<typename SaveT, typename OpT>
struct Save {
	Save(SaveT &output, OpT op):
		m_output(output),
		m_op(op) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		if (!apply(value, position, m_op))
			return false;
		m_output = value.substr(savedPosition, position - savedPosition);
		return true;
	}
private:
	SaveT &m_output;
	OpT m_op;
};
template<typename OpT>
struct Save<std::size_t, OpT> {
	Save(std::size_t &start, std::size_t &end, OpT op):
		m_start(start),
		m_end(end),
		m_op(op) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		if (!apply(value, position, m_op))
			return false;
		m_start = savedPosition;
		m_end = position;
		return true;
	}
private:
	std::size_t &m_start, &m_end;
	OpT m_op;
};
template<typename SetT>
struct Set {
	Set(SetT &output, SetT value):
		m_output(output),
		m_value(value) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		m_output = m_value;
		return true;
	}
private:
	SetT &m_output;
	SetT m_value;
};
template<>
struct Save<bool, void> {
	Save(bool &visited, bool value = true):
		m_visited(visited),
		m_value(value) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		m_visited = m_value;
		return true;
	}
private:
	bool &m_visited;
	bool m_value;
};
template<typename OpT>
struct Count {
	Count(OpT op, std::size_t count):
		m_op(op),
		m_from(count),
		m_to(count) {
	}
	Count(OpT op, std::size_t from, std::size_t to):
		m_op(op),
		m_from(from),
		m_to(to) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		for (std::size_t i = 0; i < m_to; i++) {
			if (!apply(value, position, m_op)) {
				if (i >= m_from)
					return true;
				position = savedPosition;
				return false;
			}
		}
		return true;
	}
private:
	OpT m_op;
	std::size_t m_from, m_to;
};
template<typename OpT>
struct OneOrMore {
	OneOrMore(OpT op):
		m_op(op) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		bool matchedOne = false;
		for (;;) {
			if (apply(value, position, m_op)) {
				matchedOne = true;
			} else {
				if (!matchedOne) {
					position = savedPosition;
					return false;
				}
				return true;
			}
		}
		return true;
	}
private:
	OpT m_op;
};
template<typename OpT>
struct ZeroOrMore {
	ZeroOrMore(OpT op):
		m_op(op) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		for (;;) {
			if (position >= value.length())
				return true;
			if (!apply(value, position, m_op)) {
				position = savedPosition;
				return true;
			} else {
				savedPosition = position;
			}
		}
		return true;
	}
private:
	OpT m_op;
};
template<typename OpT>
struct Optional {
	Optional(OpT op):
		m_op(op) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		apply(value, position, m_op);
		return true;
	}
private:
	OpT m_op;
};
template<typename... OpTs>
struct Or {
	Or(OpTs &&...ops):
		m_ops(std::forward_as_tuple(ops...)) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		static constexpr auto size = std::tuple_size<std::tuple<OpTs...>>::value;
		if (!applyOr(value, position, m_ops, std::make_index_sequence<size> {})) {
			position = savedPosition;
			return false;
		}
		return true;
	}
private:
	std::tuple<OpTs...> m_ops;
};
template<typename... OpTs>
struct Sequence {
	Sequence(OpTs &&...ops):
		m_ops(std::forward_as_tuple(ops...)) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		static constexpr auto size = std::tuple_size<std::tuple<OpTs...>>::value;
		if (!applyAnd(value, position, m_ops, std::make_index_sequence<size> {})) {
			position = savedPosition;
			return false;
		}
		return true;
	}
private:
	std::tuple<OpTs...> m_ops;
};
template<typename... OpTs>
struct Choice {
	Choice(std::size_t &active, OpTs &&...ops):
		m_active(active),
		m_ops(std::forward_as_tuple(ops...)) {
	}
	template<typename T>
	bool operator()(T value, std::size_t &position) const {
		std::size_t savedPosition = position;
		static constexpr auto size = std::tuple_size<std::tuple<OpTs...>>::value;
		if (!applyByIndex(m_active, value, position, m_ops, std::make_index_sequence<size> {})) {
			position = savedPosition;
			return false;
		}
		return true;
	}
private:
	std::size_t &m_active;
	std::tuple<OpTs...> m_ops;
};
template<typename T, typename OpT>
bool matchPattern(T value, std::size_t position, OpT op) {
	return apply(value, position, op);
}
template<typename T, typename OpT, typename... Args>
bool matchPattern(T value, std::size_t position, OpT op, Args... args) {
	if (!apply(value, position, op))
		return false;
	return matchPattern(value, position, args...);
}
}
template<typename T, typename... Args>
bool matchPattern(T value, Args... args) {
	return detail::matchPattern(value, 0, args...);
}
namespace ops {
const auto end = detail::End();
template<typename CharT>
auto single(CharT value) {
	return detail::CharOp<CharT>(value);
}
template<typename CharT, std::size_t Count>
auto single(const CharT (&values)[Count]) {
	return detail::CharsOp<CharT, Count>(values, std::make_index_sequence<Count> {});
}
template<typename StartOpT, typename MatchOpT>
auto startWith(StartOpT startOp, MatchOpT matchOp) {
	if constexpr (std::is_same_v<StartOpT, char> || std::is_same_v<StartOpT, wchar_t>) {
		return detail::StartOp<detail::CharOp<StartOpT>, MatchOpT>(detail::CharOp(startOp), matchOp);
	} else if constexpr (std::is_same_v<StartOpT, std::basic_string_view<char>> || std::is_same_v<StartOpT, std::basic_string<char>> || std::is_same_v<StartOpT, std::basic_string_view<wchar_t>> || std::is_same_v<StartOpT, std::basic_string<wchar_t>>) {
		return detail::StartOp<detail::StringOp<StartOpT>, MatchOpT>(detail::StringOp(startOp), matchOp);
	} else if constexpr (std::is_same_v<StartOpT, const char *>) {
		return detail::StartOp<detail::StringOp<std::string_view>, MatchOpT>(detail::StringOp(std::string_view(startOp)), matchOp);
	} else if constexpr (std::is_same_v<StartOpT, const wchar_t *>) {
		return detail::StartOp<detail::StringOp<std::wstring_view>, MatchOpT>(detail::StringOp(std::wstring_view(startOp)), matchOp);
	} else {
		return detail::StartOp<StartOpT, MatchOpT>(startOp, matchOp);
	}
}
template<typename StartOpT>
auto startWith(StartOpT startOp) {
	if constexpr (std::is_same_v<StartOpT, char> || std::is_same_v<StartOpT, wchar_t>) {
		return detail::StartOp<detail::CharOp<StartOpT>, void>(detail::CharOp(startOp));
	} else if constexpr (std::is_same_v<StartOpT, std::basic_string_view<char>> || std::is_same_v<StartOpT, std::basic_string<char>> || std::is_same_v<StartOpT, std::basic_string_view<wchar_t>> || std::is_same_v<StartOpT, std::basic_string<wchar_t>>) {
		return detail::StartOp<detail::StringOp<StartOpT>, void>(detail::StringOp(startOp));
	} else if constexpr (std::is_same_v<StartOpT, const char *>) {
		return detail::StartOp<detail::StringOp<std::string_view>, void>(detail::StringOp(std::string_view(startOp)));
	} else if constexpr (std::is_same_v<StartOpT, const wchar_t *>) {
		return detail::StartOp<detail::StringOp<std::wstring_view>, void>(detail::StringOp(std::wstring_view(startOp)));
	} else {
		return detail::StartOp<StartOpT, void>(startOp);
	}
}
template<typename CharT>
auto range(CharT from, CharT to) {
	return detail::CharRangeOp<CharT>(from, to);
}
const auto digit = detail::DecOp();
const auto hex = detail::HexOp();
const auto whitespace = detail::WhitespaceOp();
template<typename OpT>
inline auto count(OpT op, std::size_t from, std::size_t to) {
	return detail::Count<OpT>(op, from, to);
}
template<typename OpT>
inline auto count(OpT op, std::size_t count) {
	return detail::Count<OpT>(op, count);
}
template<typename OpT>
inline auto oneOrMore(OpT op) {
	return detail::OneOrMore<OpT>(op);
}
template<typename OpT>
inline auto zeroOrMore(OpT op) {
	return detail::ZeroOrMore<OpT>(op);
}
template<typename OpT>
inline auto optional(OpT op) {
	return detail::Optional<OpT>(op);
}
template<typename... OpTs>
inline auto opOr(OpTs... ops) {
	return detail::Or<OpTs...>(std::forward<OpTs>(ops)...);
}
template<typename SaveT, typename OpT>
inline auto save(OpT op, SaveT &output) {
	return detail::Save<SaveT, OpT>(output, op);
}
template<typename OpT>
inline auto save(OpT op, std::size_t &start, std::size_t &end) {
	return detail::Save<std::size_t, OpT>(start, end, op);
}
inline auto save(bool &visited, bool value = true) {
	return detail::Save<bool, void>(visited, value);
}
template<typename T>
inline auto set(T &output, T value) {
	return detail::Set<T>(output, value);
}
template<typename... OpTs>
inline auto sequence(OpTs... ops) {
	return detail::Sequence<OpTs...>(std::forward<OpTs>(ops)...);
}
template<typename... OpTs>
inline auto choice(std::size_t &active, OpTs... ops) {
	return detail::Choice<OpTs...>(active, std::forward<OpTs>(ops)...);
}
const auto maybeSpace = zeroOrMore(whitespace);
const auto maybeSpaceStrict = zeroOrMore(single(' '));
const auto space = oneOrMore(whitespace);
const auto number = sequence(optional(single({'+', '-'})), opOr(sequence(oneOrMore(digit), single('.'), oneOrMore(digit)), sequence(single('.'), oneOrMore(digit)), oneOrMore(digit)), optional(sequence(single({'e', 'E'}), oneOrMore(digit))));
}
}
