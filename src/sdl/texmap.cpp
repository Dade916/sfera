/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of Sfera.                                           *
 *                                                                         *
 *   Sfera is free software; you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Sfera is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#if defined (WIN32)
#include <windows.h>
#endif

#include <FreeImage.h>

#include "sfera.h"
#include "sdl/texmap.h"

TextureMap::TextureMap(const std::string &fileName) {
	SFERA_LOG("Reading texture map: " << fileName);

	string name = "gamedata/" + fileName;
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(name.c_str(), 0);
	if(fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(name.c_str());

	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		FIBITMAP *dib = FreeImage_Load(fif, name.c_str(), 0);

		if (!dib)
			throw std::runtime_error("Unable to read texture map: " + name);

		width = FreeImage_GetWidth(dib);
		height = FreeImage_GetHeight(dib);

		unsigned int pitch = FreeImage_GetPitch(dib);
		FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(dib);
		unsigned int bpp = FreeImage_GetBPP(dib);
		BYTE *bits = (BYTE *)FreeImage_GetBits(dib);

		if ((imageType == FIT_RGBAF) && (bpp == 128)) {
			SFERA_LOG("HDR RGB (128bit) texture map size: " << width << "x" << height << " (" <<
					width * height * sizeof(Spectrum) / 1024 << "Kbytes)");
			pixels = new Spectrum[width * height];
			alpha = NULL;

			for (unsigned int y = 0; y < height; ++y) {
				FIRGBAF *pixel = (FIRGBAF *)bits;
				for (unsigned int x = 0; x < width; ++x) {
					const unsigned int offset = x + (height - y - 1) * width;
					pixels[offset].r = pixel[x].red;
					pixels[offset].g = pixel[x].green;
					pixels[offset].b = pixel[x].blue;
				}

				// Next line
				bits += pitch;
			}
		} else if ((imageType == FIT_RGBF) && (bpp == 96)) {
			SFERA_LOG("HDR RGB (96bit) texture map size: " << width << "x" << height << " (" <<
					width * height * sizeof(Spectrum) / 1024 << "Kbytes)");
			pixels = new Spectrum[width * height];
			alpha = NULL;

			for (unsigned int y = 0; y < height; ++y) {
				FIRGBF *pixel = (FIRGBF *)bits;
				for (unsigned int x = 0; x < width; ++x) {
					const unsigned int offset = x + (height - y - 1) * width;
					pixels[offset].r = pixel[x].red;
					pixels[offset].g = pixel[x].green;
					pixels[offset].b = pixel[x].blue;
				}

				// Next line
				bits += pitch;
			}
		} else if ((imageType == FIT_BITMAP) && (bpp == 32)) {
			SFERA_LOG("RGBA texture map size: " << width << "x" << height << " (" <<
					width * height * (sizeof(Spectrum) + sizeof(float)) / 1024 << "Kbytes)");
			const unsigned int pixelCount = width * height;
			pixels = new Spectrum[pixelCount];
			alpha = new float[pixelCount];

			for (unsigned int y = 0; y < height; ++y) {
				BYTE *pixel = (BYTE *)bits;
				for (unsigned int x = 0; x < width; ++x) {
					const unsigned int offset = x + (height - y - 1) * width;
					pixels[offset].r = pixel[FI_RGBA_RED] / 255.f;
					pixels[offset].g = pixel[FI_RGBA_GREEN] / 255.f;
					pixels[offset].b = pixel[FI_RGBA_BLUE] / 255.f;
					alpha[offset] = pixel[FI_RGBA_ALPHA] / 255.f;
					pixel += 4;
				}

				// Next line
				bits += pitch;
			}
		} else if (bpp == 24) {
			SFERA_LOG("RGB texture map size: " << width << "x" << height << " (" <<
					width * height * sizeof(Spectrum) / 1024 << "Kbytes)");
			pixels = new Spectrum[width * height];
			alpha = NULL;

			for (unsigned int y = 0; y < height; ++y) {
				BYTE *pixel = (BYTE *)bits;
				for (unsigned int x = 0; x < width; ++x) {
					const unsigned int offest = x + (height - y - 1) * width;
					pixels[offest].r = pixel[FI_RGBA_RED] / 255.f;
					pixels[offest].g = pixel[FI_RGBA_GREEN] / 255.f;
					pixels[offest].b = pixel[FI_RGBA_BLUE] / 255.f;
					pixel += 3;
				}

				// Next line
				bits += pitch;
			}
		} else if (bpp == 8) {
			SFERA_LOG("Converting 8bpp image to 24bpp RGB texture map size: " << width << "x" << height << " (" <<
					width * height * sizeof (Spectrum) / 1024 << "Kbytes)");

			pixels = new Spectrum[width * height];
			alpha = NULL;

			for (unsigned int y = 0; y < height; ++y) {
				BYTE pixel;
				for (unsigned int x = 0; x < width; ++x) {
					FreeImage_GetPixelIndex(dib, x, y, &pixel);
					const unsigned int offest = x + (height - y - 1) * width;
					pixels[offest].r = pixel / 255.f;
					pixels[offest].g = pixel / 255.f;
					pixels[offest].b = pixel / 255.f;
				}

				// Next line
				bits += pitch;
			}
		} else {
			std::stringstream msg;
			msg << "Unsupported bitmap depth (" << bpp << ") in a texture map: " << fileName;
			throw std::runtime_error(msg.str());
		}

		FreeImage_Unload(dib);
	} else
		throw std::runtime_error("Unknown image file format: " + fileName);

	DuDv.u = 1.f / width;
	DuDv.v = 1.f / height;
}

TextureMap::TextureMap(Spectrum *cols, const unsigned int w, const unsigned int h) {
	pixels = cols;
	alpha = NULL;
	width = w;
	height = h;

	DuDv.u = 1.f / width;
	DuDv.v = 1.f / height;
}

TextureMap::~TextureMap() {
	delete[] pixels;
	delete[] alpha;
}

TextureMap::TextureMap(const std::string& baseFileName, const float red, const float green, const float blue) {
	SFERA_LOG("Creating blank texture from: " << baseFileName);

	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(baseFileName.c_str(), 0);
	if (fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(baseFileName.c_str());

	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		FIBITMAP *dib = FreeImage_Load(fif, baseFileName.c_str(), 0);

		if (!dib)
			throw std::runtime_error("Unable to read base texture map: " + baseFileName);

		width = FreeImage_GetWidth(dib);
		height = FreeImage_GetHeight(dib);
		FreeImage_Unload(dib);

		const unsigned int numPixels = width*height;
		pixels = new Spectrum[numPixels];
		alpha = NULL;

		SFERA_LOG("Initializing the texture with " << red << ", " << green << ", " << blue << " for " << numPixels << " pixels");
		for (unsigned int i = 0; i < numPixels; i++) {
			pixels[i].r = red;
			pixels[i].g = green;
			pixels[i].b = blue;
		}
	}
	DuDv.u = 1.f / width;
	DuDv.v = 1.f / height;
}

void TextureMap::AddAlpha(const std::string &alphaMapFileName) {
	// Don't overwrite a pre-existing alpha
	if (alpha != NULL) {
		return;
	}

	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(alphaMapFileName.c_str(), 0);
	if (fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(alphaMapFileName.c_str());

	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		FIBITMAP *dib = FreeImage_Load(fif, alphaMapFileName.c_str(), 0);

		if (!dib)
			throw std::runtime_error("Unable to read alpha map: " + alphaMapFileName);

		unsigned int alphaWidth = FreeImage_GetWidth(dib);
		unsigned int alphaHeight = FreeImage_GetHeight(dib);

		unsigned int pitch = FreeImage_GetPitch(dib);
		unsigned int bpp = FreeImage_GetBPP(dib);
		BYTE *bits = (BYTE *) FreeImage_GetBits(dib);

		const unsigned int pixelCount = alphaWidth * alphaHeight;
		alpha = new float[pixelCount];
		if (alpha == NULL) {
			SFERA_LOG("Error: could not allocate alpha channel for map " << alphaMapFileName);
			return;
		}

		unsigned short int pixelIncrement = bpp / 8; // I'm well aware of the assumption risk. Alphamaps are usually RGB or greyscale (1bpp)
		for (unsigned int y = 0; y < alphaHeight; ++y) {
			BYTE *pixel = (BYTE *) bits;
			for (unsigned int x = 0; x < alphaWidth; ++x) {
				const unsigned int offset = x + (alphaHeight - y - 1) * alphaWidth;
				alpha[offset] = *pixel / 255.f;
				pixel += pixelIncrement;
			}

			// Next line
			bits += pitch;
		}

		FreeImage_Unload(dib);
	}
}

TextureMapCache::TextureMapCache() {
}

TextureMapCache::~TextureMapCache() {
	for (size_t i = 0; i < texInstances.size(); ++i)
		delete texInstances[i];
	for (size_t i = 0; i < bumpInstances.size(); ++i)
		delete bumpInstances[i];

	for (std::map<std::string, TextureMap *>::const_iterator it = maps.begin(); it != maps.end(); ++it)
		delete it->second;
}

TextureMap *TextureMapCache::GetTextureMap(const std::string &fileName) {
	// Check if the texture map has been already loaded
	std::map<std::string, TextureMap *>::const_iterator it = maps.find(fileName);

	if (it == maps.end()) {
		// I have yet to load the file

		TextureMap *tm = new TextureMap(fileName);
		maps.insert(std::make_pair(fileName, tm));

		return tm;
	} else {
		//SFERA_LOG("Cached texture map: " << fileName);
		return it->second;
	}
}

TexMapInstance *TextureMapCache::GetTexMapInstance(const std::string &fileName,
		const float shiftU, const float shiftV, const float scaleU, const float scaleV) {
	TextureMap *tm = GetTextureMap(fileName);
	TexMapInstance *texm = new TexMapInstance(tm, shiftU, shiftV, scaleU, scaleV);
	texInstances.push_back(texm);

	return texm;
}

BumpMapInstance *TextureMapCache::GetBumpMapInstance(const std::string &fileName, const float scale) {
	TextureMap *tm = GetTextureMap(fileName);
	BumpMapInstance *bm = new BumpMapInstance(tm, scale);
	bumpInstances.push_back(bm);

	return bm;
}

void TextureMapCache::GetTexMaps(std::vector<TextureMap *> &tms) {
	for (std::map<std::string, TextureMap *>::const_iterator it = maps.begin(); it != maps.end(); ++it)
		tms.push_back(it->second);
}
