/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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

#include "ChangeNotification.h"

using namespace std;


NotificationLink::NotificationLink(const char *source_name_, uint32_t source_slot_id_, const char *destination_name_, uint32_t destination_slot_id_){
    source_name = source_name_;
    destination_name = destination_name_;
    source_slot_id = source_slot_id_;
    destination_slot_id = destination_slot_id_;
	enabled = false;
}

ChangeNotification::ChangeNotification(){

}

ChangeNotification::~ChangeNotification(){

}

bool ChangeNotification::registerSource(const char *location, ColorSource *source){
	sources[location] = source;
	return true;
}

bool ChangeNotification::unregisterSource(const char *location, ColorSource *source){
    map<string, ColorSource*>::iterator i = sources.find(location);
	if (i != sources.end()){
		sources.erase(i);
		return true;
	}
	return false;
}

bool ChangeNotification::addLink(shared_ptr<NotificationLink> notification_link){
	links.insert(pair<string, shared_ptr<NotificationLink> >(notification_link->source_name, notification_link));
	return true;
}

bool ChangeNotification::removeLink(shared_ptr<NotificationLink> notification_link){

	return true;
}


