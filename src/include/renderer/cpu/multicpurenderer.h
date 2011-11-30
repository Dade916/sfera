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

#ifndef _SFERA_MULTICPURENDERER_H
#define	_SFERA_MULTICPURENDERER_H

#include "utils/randomgen.h"
#include "renderer/levelrenderer.h"
#include "pixel/framebuffer.h"
#include "acceleretor/acceleretor.h"
#include "renderer/cpu/cpurenderer.h"

class MultiCPURendererThread;

class MultiCPURenderer : public CPURenderer {
public:
	MultiCPURenderer(GameLevel *level);
	~MultiCPURenderer();

	size_t DrawFrame();

	friend class MultiCPURendererThread;

private:
	size_t threadCount;
	vector<MultiCPURendererThread *> renderThread;
	boost::barrier *barrier;

	vector<FrameBuffer *> threadPassFrameBuffer;

	Accelerator *accel;
	PerspectiveCamera cameraCopy;
};

class MultiCPURendererThread {
public:
	MultiCPURendererThread(const size_t threadIndex, MultiCPURenderer *multiCPURenderer);
	~MultiCPURendererThread();

	void Start();
	void Stop();

private:
	static void MultiCPURenderThreadImpl(MultiCPURendererThread *renderThread);

	size_t index;
	boost::thread *renderThread;

	MultiCPURenderer *renderer;
	RandomGenerator rnd;
};

#endif	/* _SFERA_MULTICPURENDERER_H */
