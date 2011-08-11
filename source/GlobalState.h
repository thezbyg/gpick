/*
 * Copyright (c) 2009-2011, Albertas Vyšniauskas
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

#ifndef GLOBALSTATE_H_
#define GLOBALSTATE_H_

typedef struct GlobalState GlobalState;

enum GlobalStateLevel{
	GLOBALSTATE_CONFIGURATION = 1,
	GLOBALSTATE_SCRIPTING = 2,
	GLOBALSTATE_COLOR_LIST = 4,
	GLOBALSTATE_CONVERTERS = 8,
	GLOBALSTATE_COLOR_NAMES = 16,
	GLOBALSTATE_OTHER = 32,
	GLOBALSTATE_TRANSFORMATIONS = 64,
	GLOBALSTATE_ALL = 0xffffffff,
};

int global_state_init(GlobalState *gs, GlobalStateLevel level);
int global_state_term(GlobalState *gs);
GlobalState *global_state_create();
int global_state_destroy(GlobalState* gs);

#endif /* GLOBALSTATE_H_ */
