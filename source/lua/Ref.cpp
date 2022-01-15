/*
 * Copyright (c) 2009-2017, Albertas Vyšniauskas
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

#include "Ref.h"
#include <lualib.h>
#include <lauxlib.h>
namespace lua
{
Ref::Ref():
	m_ref(LUA_NOREF),
	m_L(nullptr)
{
}
Ref::Ref(lua_State *L, int index)
{
	lua_pushvalue(L, index);
	m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	m_L = L;
}
Ref::Ref(Ref &&ref)
{
	m_ref = ref.m_ref;
	m_L = ref.m_L;
	ref.m_ref = LUA_NOREF;
	ref.m_L = nullptr;
}
Ref::~Ref()
{
	if (m_L && m_ref != LUA_NOREF)
		luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
}
void Ref::get()
{
	if (!m_L)
		return;
	lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref);
}
lua_State *Ref::script()
{
	return m_L;
}
Ref &Ref::operator=(Ref &&ref)
{
	m_ref = ref.m_ref;
	m_L = ref.m_L;
	ref.m_ref = LUA_NOREF;
	ref.m_L = nullptr;
	return *this;
}
bool Ref::valid() const
{
	return m_L && m_ref != LUA_NOREF;
}
}
