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

#ifndef TRANSFORMATION_H_
#define TRANSFORMATION_H_

#include "../Color.h"
#include "Configuration.h"
#include "../DynvHelpers.h"
#include <string>
#include <boost/shared_ptr.hpp>

/** \file source/transformation/Transformation.h
 * \brief Color transformation class.
 */

namespace transformation {

/** \class Transformation
 * \brief Transformation object class.
 */
class Transformation{
	protected:
		std::string name;             /**< System name */
		std::string readable_name;    /**< Human readable name */

		/**
		 * Apply transformation to color.
		 * @param[in] input Source color in RGB color space.
		 * @param[out] output Destination color in RGB color space.
		 */
		virtual void apply(Color *input, Color *output);
	public:
		/**
		 * Transformation object constructor.
		 * @param[in] name Transformation object system name.
		 * @param[in] readable_name Transformation object human readable name.
		 */
		Transformation(const char *name, const char *readable_name);

		/**
		 * Transformation object destructor.
		 */
		virtual ~Transformation();

		/**
		 * Serialize settings into configuration system.
		 * @param[in,out] dynv Configuration system.
		 */
		virtual void serialize(struct dynvSystem *dynv);

		/**
		 * Deserialize settings from configuration system.
		 * @param[in] dynv Configuration system.
		 */
		virtual void deserialize(struct dynvSystem *dynv);

		/**
		 * Get configuration for transformation object.
		 * @return Configuration for transformation object.
		 */
		virtual boost::shared_ptr<Configuration> getConfig();

		/**
		 * Get transformation object system name.
		 * @return Transformation object system name.
		 */
		std::string getName() const;

		/**
		 * Get transformation object human readable name.
		 * @return Transformation object human readable name.
		 */
		std::string getReadableName() const;

		friend class Chain;
};

}

#endif /* TRANSFORMATION_H_ */

