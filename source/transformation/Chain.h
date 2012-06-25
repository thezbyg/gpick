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

#ifndef TRANSFORMATION_CHAIN_H_
#define TRANSFORMATION_CHAIN_H_

#include "Transformation.h"
#include <boost/shared_ptr.hpp>
#include <list>

/** \file source/transformation/Chain.h
 * \brief Class for transformation object list handling.
 */

namespace transformation {

/** \class Chain
 * \brief Transformation object chain management class.
 */
class Chain{
	public:
		typedef std::list<boost::shared_ptr<Transformation> > TransformationList;
	protected:
		TransformationList transformation_chain;
		bool enabled;
	public:
		/**
		 * Chain constructor.
		 */
		Chain();

		/**
		 * Apply transformation chain to color.
		 * @param[in] input Source color in RGB color space.
		 * @param[out] output Destination color in RGB color space.
		 */
		void apply(const Color *input, Color *output);

		/**
		 * Add transformation object into the list.
		 * @param[in] transformation Transformation object.
		 */
		void add(boost::shared_ptr<Transformation> transformation);

		/**
		 * Remove transformation object from the list.
		 * @param[in] transformation Transformation object.
		 */
		void remove(const Transformation *transformation);

		/**
		 * Clear transformation object list.
		 */
		void clear();

		/**
		 * Enable/disable transformation chain.
		 * @param[in] enabled Enabled.
		 */
		void setEnabled(bool enabled);

		/**
		 * Get the list of transformation objects.
		 * @return Transformation object list.
		 */
		TransformationList& getAll();
};

}

#endif /* TRANSFORMATION_CHAIN_H_ */

