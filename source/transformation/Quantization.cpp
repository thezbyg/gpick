/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
 * All rights reserved.
 * Copyright (c) 2012, David Gowers (Portions regarding adaptation of GammaModification.(cpp|h) to quantise colors instead.)
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

#include "Quantization.h"
#include "dynv/Map.h"
#include "uiUtilities.h"
#include "I18N.h"
#include <cmath>
#include <gtk/gtk.h>
namespace transformation {
Color Quantization::apply(Color input) {
	int multiplier = m_steps;
	int divider = m_steps - 1;
	if (m_linearization)
		input.linearRgbInplace();
	input.red = std::min<float>(std::floor(input.red * multiplier), divider) / divider;
	input.green = std::min<float>(std::floor(input.green * multiplier), divider) / divider;
	input.blue = std::min<float>(std::floor(input.blue * multiplier), divider) / divider;
	if (m_linearization)
		input.nonLinearRgbInplace();
	input.alpha = input.alpha;
	return input;
}
Quantization::Quantization():
	m_steps(16),
	m_linearization(false) {
}
void Quantization::serialize(dynv::Map &system) {
	Transformation::serialize(system);
	system.set("steps", m_steps);
	system.set("linearization", m_linearization);
}
void Quantization::deserialize(const dynv::Map &system) {
	m_steps = system.getInt32("steps", 16);
	m_linearization = system.getBool("linearization", false);
}
std::unique_ptr<BaseConfiguration> Quantization::configuration(IEventHandler &eventHandler) {
	return std::make_unique<Configuration>(eventHandler, *this);
}
Quantization::Configuration::Configuration(IEventHandler &eventHandler, Quantization &transformation):
	BaseConfiguration(eventHandler, transformation) {
	Grid grid(2, 2);
	grid.addLabel(_("Steps:"));
	grid.add(m_steps = gtk_spin_button_new_with_range(2, 256, 1), true);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_steps), transformation.m_steps);
	g_signal_connect(G_OBJECT(m_steps), "value-changed", G_CALLBACK(onChange), this);
	grid.nextColumn();
	grid.add(m_linearization = newCheckbox(_("Linearization"), transformation.m_linearization), true);
	g_signal_connect(G_OBJECT(m_linearization), "toggled", G_CALLBACK(onChange), this);
	setContent(grid);
}
void Quantization::Configuration::apply(Transformation &transformation) {
	auto &quantization = dynamic_cast<Quantization &>(transformation);
	quantization.m_steps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_steps));
	quantization.m_linearization = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_linearization));
}
}
