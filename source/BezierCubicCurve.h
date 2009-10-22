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

#ifndef BEZIERCUBICCURVE_H_
#define BEZIERCUBICCURVE_H_

namespace math{
template<typename PT, typename T>
class BezierCubicCurve{
public:

	BezierCubicCurve(const PT &p0_, const PT &p1_, const PT &p2_, const PT &p3_):p0(p0_),p1(p1_),p2(p2_),p3(p3_){
	};
	
	PT operator() (const T &t){
		T t2 = 1-t;		
		return p0*(t2*t2*t2) + p1*(3*(t2*t2)*t) + p2*(3*t2*t*t) + p3*(t*t*t);
	};
	
	BezierCubicCurve& operator= (const BezierCubicCurve& curve){
		p0 = curve.p0;
		p1 = curve.p1;
		p2 = curve.p2;
		p3 = curve.p3;			
		return *this;
	};
	
	PT p0;
	PT p1;
	PT p2;
	PT p3;
};


}

#endif /* BEZIERCUBICCURVE_H_ */
