/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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

#include "DynvXml.h"
#include "DynvVariable.h"
#include "expat.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <stack>
using namespace std;

int dynv_xml_serialize(struct dynvSystem* dynv_system, ostream& out){
	struct dynvVariable *variable, *v;
	
	
	for (dynvSystem::VariableMap::iterator i=dynv_system->variables.begin(); i!=dynv_system->variables.end(); ++i){
		variable=(*i).second;
		
		if (variable->flags & dynvVariable::NO_SAVE) continue;
		
		if (variable->handler->serialize_xml){
			if (variable->next){
				out << "<" << variable->name << " type=\"" << variable->handler->name << "\" list=\"true\">";
				v = variable;
				while (v){
					out << "<li>";
					v->handler->serialize_xml(v, out);
					out << "</li>";	
					v = v->next;
				}				
				out << "</" << variable->name << ">" << endl;
			}else{
				out << "<" << variable->name << " type=\"" << variable->handler->name << "\">";
				variable->handler->serialize_xml(variable, out);
				out << "</" << variable->name << ">" << endl;
			}
		}
		
	}
	return 0;
}

class XmlEntity{
public:
	stringstream entity_data;
	struct dynvVariable *variable;
	struct dynvSystem* dynv;
	bool list_expected;
	struct dynvHandler *list_handler;
	bool first_item;

	XmlEntity(struct dynvVariable *_variable, struct dynvSystem* _dynv, bool _list_expected):variable(_variable),dynv(_dynv),entity_data(stringstream::out),list_expected(_list_expected){
		list_handler = 0;
		first_item = true;
	};
};

class XmlCtx{
public:
	bool root_found;
	stack<XmlEntity*> entity;
	struct dynvHandlerMap *handler_map;
	
	XmlCtx(){
		root_found = false;
		handler_map = 0;
	};
	
	~XmlCtx(){
		if (handler_map) dynv_handler_map_release(handler_map);
		
		for (; ! entity.empty(); entity.pop())
			if (entity.top()) delete entity.top();
	}
};

static char* get_attribute(const XML_Char **atts, const char *attribute){
	XML_Char **i = (XML_Char**)atts;
	while (*i){
		if (strcmp(attribute, *i)==0){
			return *(i+1);
		}
		i+=2;
	}
	return 0;
}

static void start_element_handler(XmlCtx *xml, const XML_Char *name, const XML_Char **atts){
	if (xml->root_found){
		
		XmlEntity *entity = xml->entity.top();
		if (!entity) return;
		
		struct dynvVariable *variable;
		XmlEntity *n;

		if (entity->list_expected){
			if (name && strcmp(name, "li")==0){
				if (entity->first_item){
					xml->entity.push(n = new XmlEntity(entity->variable, entity->dynv, false));
					entity->first_item = false;
				}else{
					variable = dynv_variable_create(0, entity->list_handler);
					variable->handler->create(variable);
					xml->entity.push(n = new XmlEntity(variable, entity->dynv, false));
					
					entity->variable->next = variable;
					entity->variable = variable;
				}
				
			}else{
				xml->entity.push(0);
			}
		}else{
		
			char* type = get_attribute(atts, "type");
			char* list = get_attribute(atts, "list");
			
			struct dynvHandler *handler = dynv_handler_map_get_handler(xml->handler_map, type);
			
			if (handler){
				if (strcmp(type, "dynv")==0){
					struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(entity->dynv);
					struct dynvSystem* dlevel_new = dynv_system_create(handler_map);
					dynv_handler_map_release(handler_map);
					
					if (variable = dynv_system_add_empty(entity->dynv, handler, name)){
						handler->set(variable, dlevel_new, false);
					}

					xml->entity.push(new XmlEntity(0, dlevel_new, false));
				}else if (handler->deserialize_xml){
					
					if (list && strcmp(list, "true")==0){
						if (variable = dynv_system_add_empty(entity->dynv, handler, name)){
							xml->entity.push(n = new XmlEntity(variable, entity->dynv, true));
							n->list_handler = handler;
						}else{
							xml->entity.push(0);
						}
					}else{
						if (variable = dynv_system_add_empty(entity->dynv, handler, name)){
							xml->entity.push(new XmlEntity(variable, entity->dynv, false));
						}else{
							xml->entity.push(0);
						}
					}
					
				}else{
					/* not deserialize'able */
					xml->entity.push(0);
				}
			}else{
				/* unknown type */
				xml->entity.push(0);	
			}
		}
		
		//cout << name << "=" << type << endl;
		
	}else{
		if (strcmp(name, "root")==0){
			XmlEntity *entity = xml->entity.top();
			
			xml->root_found = true;
			//xml->entity.push(new XmlEntity(0, entity->dynv));
		}
	}	
}

static void end_element_handler(XmlCtx *xml, const XML_Char *name){
	if (xml->root_found){
		//cout << name << endl;
		
		XmlEntity *entity = xml->entity.top();
		if (entity){
			if (entity->list_expected){
				if (entity->first_item){ 	//entry declared as list, but no list items defined
					dynv_system_remove(entity->dynv, entity->variable->name);
				}
			}else if (entity->variable){
				entity->variable->handler->deserialize_xml(entity->variable, entity->entity_data.str().c_str());
			}
			delete entity;
		}
		xml->entity.pop();
	}
}

static void character_data_handler(XmlCtx *xml, const XML_Char *s, int len){
	//cout.write(s, len);
	
	XmlEntity *entity = xml->entity.top();
	if (entity){
		entity->entity_data.write(s, len);
	}
}


int dynv_xml_deserialize(struct dynvSystem* dynv_system, istream& in){
	XML_Parser p = XML_ParserCreate("UTF-8");
	
	XML_SetElementHandler(p, (XML_StartElementHandler)start_element_handler, (XML_EndElementHandler)end_element_handler);
	XML_SetCharacterDataHandler(p, (XML_CharacterDataHandler)character_data_handler);
	
	XmlCtx ctx;
	ctx.entity.push(new XmlEntity(0, dynv_system, false));
	
	ctx.handler_map = dynv_system_get_handler_map(dynv_system);
	XML_SetUserData(p, &ctx);

	for (;;){
		void *buffer = XML_GetBuffer(p, 4096);
		
		in.read((char*)buffer, 4096);
		size_t bytes_read = in.gcount();
		
		if (!XML_ParseBuffer(p, bytes_read, bytes_read==0)) {

		}
		
		if (bytes_read == 0) break;
		
	}
	
	XML_ParserFree(p);
	return 0;
}