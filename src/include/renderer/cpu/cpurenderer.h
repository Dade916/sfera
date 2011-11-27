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

#ifndef _SFERA_CPURENDERER_H
#define	_SFERA_CPURENDERER_H

#include "gamelevel.h"
#include "utils/randomgen.h"
#include "renderer/levelrenderer.h"
#include "pixel/framebuffer.h"
#include "acceleretor/acceleretor.h"
#include "acceleretor/bvhaccel.h"

class CPURenderer : public LevelRenderer {
public:
	CPURenderer(GameLevel *level);
	~CPURenderer();

protected:
	BVHAccel *BuildAcceleretor();
	Spectrum SampleImage(
		RandomGenerator &rnd,
		const Accelerator &accel, const PerspectiveCamera &camera,
		const float screenX, const float screenY);
	void ApplyFilter();
	void BlendFrame();
	void ApplyToneMapping();
	void CopyFrame();

	FrameBuffer *passFrameBuffer;
	FrameBuffer *tmpFrameBuffer;
	FrameBuffer *frameBuffer;
	FrameBuffer *toneMapFrameBuffer;

	double timeSinceLastCameraEdit, timeSinceLastNoCameraEdit;
};

#endif	/* _SFERA_CPURENDERER_H */
