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

#include "sfera.h"
#include "displaysession.h"
#include "gamesession.h"
#include "renderer/cpu/singlecpurenderer.h"
#include "physic/gamephysic.h"

static const string SFERA_LABEL = "Sfera v" SFERA_VERSION_MAJOR "." SFERA_VERSION_MINOR " (Written by David \"Dade\" Bucciarelli)";

DisplaySession::DisplaySession(const GameConfig *cfg) : gameConfig(cfg) {
	leftMouseDown = false;
	rightMouseDown = false;
	mouseStartX = 0;
	mouseStartY = 0;
	startViewTheta = 0.f;
	startViewPhi = 0.f;

	const unsigned int width = gameConfig->GetScreenWidth();
	const unsigned int height = gameConfig->GetScreenHeight();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw std::runtime_error("Unable to initialize SDL");

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);

	SDL_putenv((char *)"SDL_VIDEO_CENTERED=center");
	SDL_WM_SetCaption(SFERA_LABEL.c_str(), NULL);

	SDL_Surface *surface = SDL_SetVideoMode(width, height, 32,
			SDL_HWSURFACE | SDL_OPENGL);
	if (!surface)
		throw std::runtime_error("Unable to create SDL window");

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, width, height);
	glLoadIdentity();
	glOrtho(0.f, width - 1.f,
			0.f, height, -1.f, 1.f);
}

DisplaySession::~DisplaySession() {
}

void DisplaySession::RunLoop() {
	GameSession gameSession(gameConfig, "Sfera");
	gameSession.LoadLevel(1);
	GameLevel *currentLevel = gameSession.currentLevel;

	GamePhysic gamePhysic(currentLevel);
	PhysicThread physicThread(&gamePhysic);

	SingleCPURenderer renderer(currentLevel);

	// Start the game

	physicThread.Start();

	unsigned int frame = 0;
	double frameStartTime = WallClockTime();
	const double refreshTime = 1.0 / gameConfig->GetScreenRefreshCap();
	double currentRefreshTime = 0.0;
	bool quit = false;
	do {
		const double t1 = WallClockTime();

		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN: {
					switch(event.button.button) {
						case SDL_BUTTON_LEFT: {
							leftMouseDown = true;
							mouseStartX = event.button.x;
							mouseStartY = event.button.y;
							startViewTheta = currentLevel->player->viewTheta;
							startViewPhi = currentLevel->player->viewPhi;
							break;
						}
						case SDL_BUTTON_RIGHT: {
							rightMouseDown = true;
							mouseStartX = event.button.x;
							mouseStartY = event.button.y;
							startViewDistance = currentLevel->player->viewDistance;
							break;
						}
					}
					break;
				}
				case SDL_MOUSEBUTTONUP: {
					switch(event.button.button) {
						case SDL_BUTTON_LEFT: {
							leftMouseDown = false;
							break;
						}
						case SDL_BUTTON_RIGHT: {
							rightMouseDown = false;
							break;
						}
					}
					break;
				}
				case SDL_MOUSEMOTION: {
					if (leftMouseDown) {
						const int deltaX = event.motion.x - mouseStartX;
						const int deltaY = event.motion.y - mouseStartY;

						currentLevel->player->viewTheta =  startViewTheta - deltaY / 200.f;
						currentLevel->player->viewPhi = startViewPhi - deltaX / 200.f;
					}

					if (rightMouseDown) {
						const int deltaY = event.motion.y - mouseStartY;

						currentLevel->player->viewDistance =  Max(
								currentLevel->player->body.sphere.rad * 5.f,
								Min(currentLevel->player->body.sphere.rad * 40.f,
									startViewDistance + deltaY / 200.f));
					}
					break;
				}
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_w:
							currentLevel->player->inputGoForward = true;
							break;
						case SDLK_a:
							currentLevel->player->inputTurnLeft = true;
							break;
						case SDLK_d:
							currentLevel->player->inputTurnRight = true;
							break;
						default:
							break;
					}
					break;
				}
				case SDL_KEYUP: {
					switch (event.key.keysym.sym) {
						case SDLK_w:
							currentLevel->player->inputGoForward = false;
							break;
						case SDLK_a:
							currentLevel->player->inputTurnLeft = false;
							break;
						case SDLK_d:
							currentLevel->player->inputTurnRight = false;
							break;
						case SDLK_ESCAPE:
							quit = true;
							break;
						default:
							break;
					}
					break;
				}
				case SDL_QUIT:
					quit = true;
					break;
				default:
					break;
			}

		}

		currentLevel->DoStep();

		renderer.DrawFrame();

		SDL_GL_SwapBuffers();

		if (currentRefreshTime < refreshTime) {
			const unsigned int sleep = (unsigned int)((refreshTime - currentRefreshTime) * 1000.0);
			boost::this_thread::sleep(boost::posix_time::millisec(sleep));
		}

		++frame;
		if (frame == 90) {
			const double now = WallClockTime();
			SFERA_LOG("Frame/sec: " << (90.0 / (now - frameStartTime)));

			frameStartTime = now;
			frame = 0;
		}

		const double t2 = WallClockTime();
		currentRefreshTime = t2 - t1;
	} while(!quit);

	SFERA_LOG("Done.");

	physicThread.Stop();
	SDL_Quit();
}
