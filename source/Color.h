/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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

#ifndef GPICK_COLOR_H_
#define GPICK_COLOR_H_

#include "math/Matrix.h"
#include "math/Vector.h"
#include <string>
#include <cstdint>

/** \file source/Color.h
 * \brief Color structure and functions to convert colors from one color space to another.
 */

/** \enum ReferenceIlluminant
 * \brief Reference illuminants.
 */
enum class ReferenceIlluminant : uint8_t {
	A = 0,
	C = 1,
	D50 = 2,
	D55 = 3,
	D65 = 4,
	D75 = 5,
	F2 = 6,
	F7 = 7,
	F11 = 8,
};

/** \enum ReferenceObserver
 * \brief Reference observers.
 */
enum class ReferenceObserver : uint8_t {
	_2 = 0,
	_10 = 1,
};

/** \struct Color
 * \brief Color structure is an union of all available color spaces.
 */
struct Color {
	using Matrix3d = math::Matrix3d;
	using Vector3f = math::Vector3f;
	using Vector3d = math::Vector3d;
	static const int MemberCount = 4;
	static const Color white;
	static const Color black;
	/**
	 * Working space matrix for sRGB.
	 */
	static const Matrix3d &sRGBMatrix;
	/**
	 * Inverted working space matrix for sRGB.
	 */
	static const Matrix3d &sRGBInvertedMatrix;
	/**
	 * D65 to D50 chromatic adaptation matrix.
	 */
	static const Matrix3d &d65d50AdaptationMatrix;
	/**
	 * D50 to D65 chromatic adaptation matrix.
	 */
	static const Matrix3d &d50d65AdaptationMatrix;
	/**
	 * Initialize things needed for color conversion functions. Must be called before using any other functions.
	 */
	static void initialize();
	Color();
	Color(const Color &color);
	Color(float value);
	Color(int value);
	Color(float red, float green, float blue, float alpha = 1.0f);
	Color(int red, int green, int blue, int alpha = 255);
	Color(const Vector3f &value, float alpha = 1.0f);
	Color(const Vector3d &value, float alpha = 1.0f);
	/**
	 * Check if colors are equal.
	 * @param[in] color Color in the same color space as current color.
	 * @return True if colors are equal.
	 */
	bool operator==(const Color &color) const;
	/**
	 * Check if colors are not equal.
	 * @param[in] color Color in the same color space as current color.
	 * @return True if colors are not equal.
	 */
	bool operator!=(const Color &color) const;
	/**
	 * Add color values.
	 * @param[in] color Color to be added to current color.
	 * @return Result of two colors added together.
	 */
	Color operator+(const Color &color) const;
	/**
	 * Subtract color values.
	 * @param[in] color Color to be subtracted from current color.
	 * @return Result of color subtracted from current color.
	 */
	Color operator-(const Color &color) const;
	/**
	 * Add color values.
	 * @param[in] color Color to be added to current color.
	 * @return Result of two colors added together.
	 */
	Color &operator+=(const Color &color);
	/**
	 * Multiply color values by value.
	 * @param[in] value Multiplier.
	 * @return Color with all values multiplied by provided value.
	 */
	Color operator*(float value) const;
	/**
	 * Multiply color values by color values.
	 * @param[in] color Color to multiply with.
	 * @return Color with all values multiplied by color values.
	 */
	Color operator*(const Color &color) const;
	/**
	 * Multiply color values by color values.
	 * @param[in] color Color to multiply with.
	 * @return Color with all values multiplied by color values.
	 */
	Color &operator*=(const Color &color);
	/**
	 * Divide color values by value.
	 * @param[in] value Divider.
	 * @return Color with all values divided by provided value.
	 */
	Color operator/(float value) const;
	/**
	 * Get color value.
	 * @param[in] index Value index.
	 * @return Color value.
	 */
	float &operator[](int index);
	/**
	 * Get color value.
	 * @param[in] index Value index.
	 * @return Color value.
	 */
	float operator[](int index) const;
	/**
	 * Zero all color values.
	 * @return Color with all values set to zero.
	 */
	Color &zero();
	/**
	 * Zero all color values.
	 * @return Color with all values set to zero.
	 */
	Color zero() const;
	/**
	 * Transform RGB color to linear RGB color.
	 * @return Linear color in RGB color space.
	 */
	Color &linearRgbInplace();
	/**
	 * Transform RGB color to linear RGB color.
	 * @return Linear color in RGB color space.
	 */
	Color linearRgb() const;
	/**
	 * Transform linear RGB color to RGB color.
	 * @return Color in RGB color space.
	 */
	Color &nonLinearRgbInplace();
	/**
	 * Transform linear RGB color to RGB color.
	 * @return Color in RGB color space.
	 */
	Color nonLinearRgb() const;
	/**
	 * Set all color values to absolute values.
	 * @return Color with absolute values.
	 */
	Color absolute() const;
	/**
	 * Set all color values to absolute values.
	 * @return Color with absolute values.
	 */
	Color &absoluteInplace();
	/**
	 * Convert RGB color space to HSL color space.
	 * @return Color in HSL color space.
	 */
	Color rgbToHsl() const;
	/**
	 * Convert HSL color space to RGB color space.
	 * @return Color in RGB color space.
	 */
	Color hslToRgb() const;
	/**
	 * Convert HSL color space to HSV color space.
	 * @return Color in HSV color space.
	 */
	Color hslToHsv() const;
	/**
	 * Convert HSV color space to HSL color space.
	 * @return Color in HSL color space.
	 */
	Color hsvToHsl() const;
	/**
	 * Convert RGB color space to HSV color space.
	 * @return Color in HSV color space.
	 */
	Color rgbToHsv() const;
	/**
	 * Convert HSV color space to RGB color space.
	 * @return Color in RGB color space.
	 */
	Color hsvToRgb() const;
	/**
	 * Convert RGB color space to XYZ color space.
	 * @param[in] transformation Transformation matrix for RGB to XYZ conversion.
	 * @return Color in XYZ color space.
	 */
	Color rgbToXyz(const Matrix3d &transformation) const;
	/**
	 * Convert XYZ color space to RGB color space.
	 * @param[in] transformationInverted Transformation matrix for XYZ to RGB conversion.
	 * @return Color in RGB color space.
	 */
	Color xyzToRgb(const Matrix3d &transformationInverted) const;
	/**
	 * Convert XYZ color space to Lab color space.
	 * @param[in] referenceWhite Reference white color values.
	 * @return Color in LAB color space.
	 */
	Color xyzToLab(const Vector3f &referenceWhite) const;
	/**
	 * Convert Lab color space to XYZ color space.
	 * @param[in] referenceWhite Reference white color values.
	 * @return Color in XYZ  color space.
	 */
	Color labToXyz(const Vector3f &referenceWhite) const;
	/**
	 * Convert RGB color space to Lab color space.
	 * @param[in] referenceWhite Reference white color values.
	 * @param[in] transformation Transformation matrix for RGB to XYZ conversion.
	 * @param[in] adaptationMatrix XYZ chromatic adaptation matrix.
	 * @return Color in LAB color space.
	 */
	Color rgbToLab(const Vector3f &referenceWhite, const Matrix3d &transformation, const Matrix3d &adaptationMatrix) const;
	/**
	 * Convert Lab color space to RGB color space.
	 * @param[in] referenceWhite Reference white color values.
	 * @param[in] transformationInverted Transformation matrix for XYZ to RGB conversion.
	 * @param[in] adaptationMatrixInverted Inverted XYZ chromatic adaptation matrix.
	 * @return Color in RGB color space.
	 */
	Color labToRgb(const Vector3f &referenceWhite, const Matrix3d &transformationInverted, const Matrix3d &adaptationMatrixInverted) const;
	/**
	 * Convert RGB color space to Lab color space with illuminant D50, observer 2, sRGB transformation matrix and D65-D50 adaptation matrix.
	 * @return Color in LAB color space.
	 */
	Color rgbToLabD50() const;
	/**
	 * Convert Lab color space to RGB color space with illuminant D50, observer 2, inverted sRGB transformation matrix and D50-D65 adaptation matrix.
	 * @return Color in RGB color space.
	 */
	Color labToRgbD50() const;
	/**
	 * Convert Lab color space to LCH color space.
	 * @return Color in LCH color space.
	 */
	Color labToLch() const;
	/**
	 * Convert Lab color space to LCH color space.
	 * @return Color in LAB color space.
	 */
	Color lchToLab() const;
	/**
	 * Convert RGB color space to LCH color space.
	 * @param[in] referenceWhite Reference white color values.
	 * @param[in] transformation Transformation matrix for RGB to XYZ conversion.
	 * @param[in] adaptationMatrix XYZ chromatic adaptation matrix.
	 * @return Color in LCH color space.
	 */
	Color rgbToLch(const Vector3f &referenceWhite, const Matrix3d &transformation, const Matrix3d &adaptationMatrix) const;
	/**
	 * Convert LCH color space to RGB color space.
	 * @param[in] referenceWhite Reference white color values.
	 * @param[in] transformationInverted Transformation matrix for XYZ to RGB conversion.
	 * @param[in] adaptationMatrixInverted Inverted XYZ chromatic adaptation matrix.
	 * @return Color in RGB color space.
	 */
	Color lchToRgb(const Vector3f &referenceWhite, const Matrix3d &transformationInverted, const Matrix3d &adaptationMatrixInverted) const;
	/**
	 * Convert RGB color space to LCH color space with illuminant D50, observer 2, sRGB transformation matrix and D65-D50 adaptation matrix.
	 * @return Color in LCH color space.
	 */
	Color rgbToLchD50() const;
	/**
	 * Convert LCH color space to RGB color space with illuminant D50, observer 2, inverted sRGB transformation matrix and D50-D65 adaptation matrix.
	 * @return Color in RGB color space.
	 */
	Color lchToRgbD50() const;
	/**
	 * Convert RGB color space to CMY color space.
	 * @return Color in CMY color space.
	 */
	Color rgbToCmy() const;
	/**
	 * Convert CMY color space to RGB color space.
	 * @return Color in RGB color space.
	 */
	Color cmyToRgb() const;
	/**
	 * Convert CMY color space to CMYK color space.
	 * @return Color in CMYK color space.
	 */
	Color cmyToCmyk() const;
	/**
	 * Convert CMYK color space to CMY color space.
	 * @return Color in CMY color space.
	 */
	Color cmykToCmy() const;
	/**
	 * Convert RGB color space to CMYK color space.
	 * @return Color in CMYK color space.
	 */
	Color rgbToCmyk() const;
	/**
	 * Convert CMYK color space to RGB color space.
	 * @return Color in RGB color space.
	 */
	Color cmykToRgb() const;
	/**
	 * Normalize RGB color values.
	 * @return Color with normalized RGB values.
	 */
	Color &normalizeRgbInplace();
	/**
	 * Normalize RGB color values.
	 * @return Color with normalized RGB values.
	 */
	Color normalizeRgb() const;
	/**
	 * Check whenever the color contains invalid (out of RGB gamut) value.
	 * @return True, when color is out of RGB gamut.
	 */
	bool isOutOfRgbGamut() const;
	/**
	 * Get either black or white color depending on which has more contrast with specified color.
	 * @return Color with most contrast in RGB color space.
	 */
	const Color &getContrasting() const;
	/**
	 * Get RGB values as vector.
	 * @return RGB value vector.
	 */
	template<typename T>
	math::Vector<T, 3> rgbVector() const;
	/**
	 * Get distance between two colors.
	 * @param[in] a First color in RGB color space.
	 * @param[in] b Second color in RGB color space.
	 * @return Distance.
	 */
	static float distance(const Color &a, const Color &b);
	/**
	 * Get distance between two colors using CIE94 color difference calculation.
	 * @param[in] a First color in LAB color space.
	 * @param[in] b Second color in LAB color space.
	 * @return Distance.
	 */
	static float distanceLch(const Color &a, const Color &b);
	/**
	 * Mix two colors together.
	 * @param[in] a First color in linear RGB color space.
	 * @param[in] b Second color in linear RGB color space.
	 * @param[in] ratio The amount of second color to mix into first color.
	 * @return Mixed color.
	 */
	static Color mix(const Color &a, const Color &b, float ratio = 0.5f);
	/**
	 * Get reference white vector for specified illuminant and observer.
	 * @param[in] illuminant Illuminant.
	 * @param[in] observer Observer.
	 * @return Reference white vector.
	 */
	static const Vector3f &getReference(ReferenceIlluminant illuminant, ReferenceObserver observer);
	/**
	 * Get illuminant by name.
	 * @param[in] illuminant Illuminant name.
	 * @return Reference illuminant.
	 */
	static ReferenceIlluminant getIlluminant(const std::string &illuminant);
	/**
	 * Get observer by name.
	 * @param[in] observer Observer name.
	 * @return Reference observer.
	 */
	static ReferenceObserver getObserver(const std::string &observer);
	/**
	 * Calculate working space matrix.
	 * @param[in] xr Red primary in x channel.
	 * @param[in] yr Red primary in y channel.
	 * @param[in] xg Green primary in x channel.
	 * @param[in] yg Green primary in y channel.
	 * @param[in] xb Blue primary in x channel.
	 * @param[in] yb Blue primary in y channel.
	 * @param[in] referenceWhite Reference white vector.
	 * @return Calculated working space matrix.
	 */
	static Matrix3d getWorkingSpaceMatrix(float xr, float yr, float xg, float yg, float xb, float yb, const Vector3f &referenceWhite);
	/**
	 * Calculate chromatic adaptation matrix from source and destination reference white vectors.
	 * @param[in] sourceReference_white Source reference white vector.
	 * @param[in] destinationReferenceWhite Destination reference white vector.
	 * @return Calculated chromatic adaptation matrix.
	 */
	static Matrix3d getChromaticAdaptationMatrix(const Vector3f &sourceReferenceWhite, const Vector3f &destinationReferenceWhite);
	/**
	 * Apply chromatic adaptation matrix to the XYZ color.
	 * @param[in] adaptation Chromatic adaptation matrix.
	 * @return Color with applied chromatic adaptation.
	 * @see getChromaticAdaptationMatrix.
	 */
	Color xyzChromaticAdaptation(const Matrix3d &adaptation) const;
	union {
		struct {
			union {
				struct {
					float red; /**< Red component */
					float green; /**< Green component */
					float blue; /**< Blue component */
				} rgb;
				struct {
					float hue;
					float saturation;
					float value;
				} hsv;
				struct {
					float hue;
					float saturation;
					float lightness;
				} hsl;
				struct {
					float x;
					float y;
					float z;
				} xyz;
				struct {
					float L;
					float a;
					float b;
				} lab;
				struct {
					float L;
					float C;
					float h;
				} lch;
				struct {
					float c;
					float m;
					float y;
				} cmy;
				struct {
					float c;
					float m;
					float y;
					float k;
				} cmyk;
			};
		};
		struct {
			float red;
			float green;
			float blue;
			float alpha;
		};
		float data[MemberCount]; /**< General data access array */
	};
};
#endif /* GPICK_COLOR_H_ */
