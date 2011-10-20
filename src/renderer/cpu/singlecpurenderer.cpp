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

#include "renderer/cpu/singlecpurenderer.h"

SingleCPURenderer::SingleCPURenderer(const GameLevel *level) : LevelRenderer(level) {
	const size_t pixelsCount = gameLevel->gameConfig->GetScreenWidth() * gameLevel->gameConfig->GetScreenHeight();
	pixels = new float[pixelsCount * 3];

	float *p = pixels;
	for (size_t i = 0; i < pixelsCount * 3; ++i)
		*p++ = 0.f;
}

SingleCPURenderer::~SingleCPURenderer() {
	delete[] pixels;
}

void SingleCPURenderer::DrawFrame() {
	const size_t pixelsCount = gameLevel->gameConfig->GetScreenWidth() * gameLevel->gameConfig->GetScreenHeight();

	float *p = pixels;
	for (size_t i = 0; i < pixelsCount * 3; ++i)
		*p++ += 0.001;

	glDrawPixels(gameLevel->gameConfig->GetScreenWidth(), gameLevel->gameConfig->GetScreenHeight(), GL_RGB, GL_FLOAT, pixels);
}
