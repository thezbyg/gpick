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

#ifndef RECT2_H_
#define RECT2_H_

namespace math{
template<typename T>
class Rect2{
public:
	Rect2(){
		empty=true;
	};
	Rect2(const T &x1_, const T &y1_, const T &x2_, const T &y2_):x1(x1_),y1(y1_),x2(x2_),y2(y2_){
		empty=false;
	};
	
	const Rect2 operator=(const Rect2 &rect){
		x1 = rect.x1;
		y1 = rect.y1;
		x2 = rect.x2;
		y2 = rect.y2;
		empty = rect.empty;
		return *this;
	};
	
	const Rect2 operator+(const Rect2 &rect) const{

		if (rect.empty) return *this;
		if (empty) return rect;
		
		Rect2 r;
			
		if (x1 < rect.x1) r.x1 = x1;
		else r.x1 = rect.x1;
		if (y1 < rect.y1) r.y1 = y1;
		else r.y1 = rect.y1;

		if (x2 > rect.x2) r.x2 = x2;
		else r.x2 = rect.x2;
		if (y2 > rect.y2) r.y2 = y2;
		else r.y2 = rect.y2;
		
		return r;
	};
	
	const Rect2 operator+=(const Rect2 &rect){
		*this=*this+rect;
		return *this;
	};
	
	const Rect2 impose(const Rect2 &rect){
		x1 = rect.x1 + x1 * rect.getWidth();
		y1 = rect.y1 + y1 * rect.getHeight();
		
		x2 = rect.x1 + x2 * rect.getWidth();
		y2 = rect.y1 + y2 * rect.getHeight();
		
		return *this;
	}
	
	bool isEmpty() const{
		return empty;
	};
	
	const T& getX() const{
		return x1;
	};
	const T& getY() const{
		return y1;
	};
	T getWidth() const{
		return x2-x1;
	};
	T getHeight() const{
		return y2-y1;
	};
	
private:
	bool empty;
	T x1, y1, x2, y2;
};


}

#endif /* RECT2_H_ */
