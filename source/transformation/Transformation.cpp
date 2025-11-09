/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

#include "Transformation.h"
#include "ColorVisionDeficiency.h"
#include "GammaModification.h"
#include "Quantization.h"
#include "Invert.h"
#include "I18N.h"
#include "dynv/Map.h"
namespace transformation {
Transformation::Transformation():
	m_description(nullptr) {
}
Color Transformation::apply(Color input) {
	return input;
}
const char *Transformation::id() const {
	return m_description->id;
}
const char *Transformation::name() const {
	return m_description->name;
}
void Transformation::setDescription(const Description &description) {
	m_description = &description;
}
std::unique_ptr<Transformation> Transformation::create(std::string_view id) {
	for (const auto &description: descriptions()) {
		if (description.id == id) {
			auto transformation = description.create();
			transformation->setDescription(description);
			return transformation;
		}
	}
	return nullptr;
}
std::unique_ptr<Transformation> Transformation::copy() const {
	return m_description->createCopy(*this);
}
void Transformation::serialize(dynv::Map &options) {
	options.set("name", m_description->id);
}
void Transformation::deserialize(const dynv::Map &options) {
}
std::unique_ptr<BaseConfiguration> Transformation::configuration(IEventHandler &eventHandler) {
	return nullptr;
}
template<typename T>
std::unique_ptr<Transformation> create() {
	return std::make_unique<T>();
}
template<typename T>
std::unique_ptr<Transformation> createCopy(const Transformation &transformation) {
	return std::make_unique<T>(dynamic_cast<const T &>(transformation));
}
static const Description descriptionArray[] = {
	{
		"color_vision_deficiency",
		_("Color vision deficiency"),
		create<ColorVisionDeficiency>,
		createCopy<ColorVisionDeficiency>,
	},
	{
		"gamma_modification",
		_("Gamma modification"),
		create<GammaModification>,
		createCopy<GammaModification>,
	},
	{
		"quantization",
		_("Quantization"),
		create<Quantization>,
		createCopy<Quantization>,
	},
	{
		"invert",
		_("Invert"),
		create<Invert>,
		createCopy<Invert>,
	},
};
common::Span<const Description> descriptions() {
	return common::Span(descriptionArray, sizeof(descriptionArray) / sizeof(descriptionArray[0]));
}
}
