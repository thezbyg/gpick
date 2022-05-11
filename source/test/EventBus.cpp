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

#include <boost/test/unit_test.hpp>
#include "EventBus.h"
BOOST_AUTO_TEST_SUITE(eventBus)
struct HandlerA: public IEventHandler {
	HandlerA(EventBus &eventBus):
		m_eventBus(eventBus) {
		m_eventBus.subscribe(EventType::optionsUpdate, *this);
	}
	virtual ~HandlerA() {
		m_eventBus.unsubscribe(*this);
	}
	virtual void onEvent(EventType) override {
	}
private:
	EventBus &m_eventBus;
};
struct HandlerB: public IEventHandler {
	HandlerB(EventBus &eventBus):
		m_eventBus(eventBus) {
		m_eventBus.subscribe(EventType::convertersUpdate, *this);
	}
	virtual ~HandlerB() {
		m_eventBus.unsubscribe(*this);
	}
	virtual void onEvent(EventType) override {
	}
private:
	EventBus &m_eventBus;
};
BOOST_AUTO_TEST_CASE(unsubscribe) {
	EventBus eventBus;
	auto a1 = std::make_unique<HandlerA>(eventBus);
	auto a2 = std::make_unique<HandlerA>(eventBus);
	auto b1 = std::make_unique<HandlerB>(eventBus);
	auto b2 = std::make_unique<HandlerB>(eventBus);
	a2.reset();
	a1.reset();
	b1.reset();
	b2.reset();
	BOOST_CHECK(eventBus.empty());
}
BOOST_AUTO_TEST_SUITE_END()
