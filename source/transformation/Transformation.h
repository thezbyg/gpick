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
#include "BaseConfiguration.h"
#include "Color.h"
#include "dynv/MapFwd.h"
#include "common/Span.h"
#include <memory>
#include <string_view>
typedef struct _GtkWidget GtkWidget;
namespace transformation {
struct Transformation;
struct IEventHandler {
	virtual void onConfigurationChange(BaseConfiguration &configuration, Transformation &transformation) = 0;
};
struct Description {
	const char *id;
	const char *name;
	std::unique_ptr<Transformation> (*create)();
	std::unique_ptr<Transformation> (*createCopy)(const Transformation &);
};
struct Transformation {
	Transformation();
	virtual ~Transformation() = default;
	virtual Color apply(Color input);
	virtual void serialize(dynv::Map &options);
	virtual void deserialize(const dynv::Map &options);
	virtual std::unique_ptr<BaseConfiguration> configuration(IEventHandler &eventHandler);
	const char *id() const;
	const char *name() const;
	static std::unique_ptr<Transformation> create(std::string_view id);
	std::unique_ptr<Transformation> copy() const;
private:
	const Description *m_description;
	void setDescription(const Description &description);
};
common::Span<const Description> descriptions();
}
