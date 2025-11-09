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

#include "GammaModification.h"
#include "dynv/Map.h"
#include "uiUtilities.h"
#include "I18N.h"
#include <gtk/gtk.h>
#include <cmath>
namespace transformation {
Color GammaModification::apply(Color input) {
	input.linearRgbInplace();
	input.red = std::pow(input.red, m_gamma);
	input.green = std::pow(input.green, m_gamma);
	input.blue = std::pow(input.blue, m_gamma);
	input.nonLinearRgbInplace().normalizeRgbInplace();
	return input;
}
GammaModification::GammaModification():
	m_gamma(1) {
}
void GammaModification::serialize(dynv::Map &system) {
	system.set("gamma", m_gamma);
	Transformation::serialize(system);
}
void GammaModification::deserialize(const dynv::Map &system) {
	m_gamma = system.getFloat("gamma", 1);
}
std::unique_ptr<BaseConfiguration> GammaModification::configuration(IEventHandler &eventHandler) {
	return std::make_unique<Configuration>(eventHandler, *this);
}
GammaModification::Configuration::Configuration(IEventHandler &eventHandler, GammaModification &transformation):
	BaseConfiguration(eventHandler, transformation) {
	Grid grid(2, 1);
	grid.addLabel(_("Gamma:"));
	grid.add(m_gamma = gtk_spin_button_new_with_range(0, 100, 0.01), true);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_gamma), transformation.m_gamma);
	g_signal_connect(G_OBJECT(m_gamma), "value-changed", G_CALLBACK(onChange), this);
	setContent(grid);
}
void GammaModification::Configuration::apply(Transformation &transformation) {
	auto &gammaModification = dynamic_cast<GammaModification &>(transformation);
	gammaModification.m_gamma = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_gamma)));
}
}
