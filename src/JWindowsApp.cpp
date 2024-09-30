//
// Created by jonas on 2024/9/29.
//
#include "JWindowsApp.h"

namespace JackalRenderer {
    JWindowsApp::ptr JWindowsApp::instance = nullptr;

    bool JWindowsApp::setup(int width, int height, string title) {
        screenWidth = width;
        screenHeight = height;
        windowTitle = title;

        lastMouseX = 0;
        lastMouseY = 0;
        mouseDeltaX = 0;
        mouseDeltaY = 0;

        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        windowHandle = SDL_CreateWindow(
            windowTitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            screenWidth,
            screenHeight,
            SDL_WINDOW_SHOWN);

        if(windowHandle == nullptr) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        screenSurface = SDL_GetWindowSurface(windowHandle);
        return true;
    }

    JWindowsApp::~JWindowsApp() {
        SDL_DestroyWindow(windowHandle);
        windowHandle = nullptr;
        SDL_Quit();
    }

    void JWindowsApp::readytoStart() {
        timer.start();
        lastTimePoint = timer.getTicks();
        fps = 0;
        fpsCounter = 0.0f;
        fpsTimeRecorder = 0.0f;
    }

    void JWindowsApp::processEvent() {
        wheelDelta = 0;
        while(SDL_PollEvent(&events) != 0) {
            if(events.type == SDL_QUIT || (events.type == SDL_KEYDOWN && events.key.keysym.sym == SDLK_ESCAPE))
                quit = true;
            if(events.type == SDL_MOUSEMOTION) {
                static bool firstEvent = true;
                if(firstEvent) {
                    firstEvent = false;
                    lastMouseX = events.motion.x;
                    lastMouseY = events.motion.y;
                    mouseDeltaX = 0;
                    mouseDeltaY = 0;
                }else {
                    mouseDeltaX = events.motion.x - lastMouseX;
                    mouseDeltaY = events.motion.y - lastMouseY;
                    lastMouseX = events.motion.x;
                    lastMouseY = events.motion.y;
                }
            }
            if(events.type == SDL_MOUSEBUTTONDOWN && events.button.button == SDL_BUTTON_LEFT) {
                mouseLeftButtonPressed = true;
                lastMouseX = events.motion.x;
                lastMouseY = events.motion.y;
                mouseDeltaX = 0;
                mouseDeltaY = 0;
            }
            if(events.type == SDL_MOUSEBUTTONUP && events.button.button == SDL_BUTTON_LEFT)
                mouseLeftButtonPressed = false;
            if(events.type == SDL_MOUSEWHEEL)
                wheelDelta = events.wheel.y;
        }
    }

    double JWindowsApp::updateScreenSurface(uchar *pixels, int width, int height, int channel, uint num_triangles) {
        SDL_LockSurface(screenSurface);
        {
            Uint32* destPixels = (Uint32*)screenSurface -> pixels;
            parallelLoop((size_t)0, (size_t)width * height, [&](const size_t& index) {
                Uint32 color = SDL_MapRGB(
                    screenSurface -> format,
                    static_cast<uint8_t>(pixels[index * channel + 0]),
                    static_cast<uint8_t>(pixels[index * channel + 1]),
                    static_cast<uint8_t>(pixels[index * channel + 2]));
                destPixels[index] = color;
            });
        }
        SDL_UnlockSurface(screenSurface);
        SDL_UpdateWindowSurface(windowHandle);

        deltaTime = timer.getTicks() - lastTimePoint;
        lastTimePoint = timer.getTicks();

        {
            fpsTimeRecorder += deltaTime;
            ++fpsCounter;
            if (fpsTimeRecorder > 1000.0f) {
                fps = static_cast<uint>(fpsCounter);
                fpsCounter = 0.0f;
                fpsTimeRecorder = 0.0f;

                std::stringstream ss;
                ss << "FPS:" << std::setiosflags(std::ios::left) << std::setw(3) << fps;
                ss << "#Triangles:" << std::setiosflags(std::ios::left) << std::setw(5) << num_triangles;
                SDL_SetWindowTitle(windowHandle, (windowTitle + ss.str()).c_str());
            }
        }
        return deltaTime;
    }

    JWindowsApp::ptr JWindowsApp::getInstance() {
        if(instance == nullptr)
            return getInstance(800, 600, "WinApp");
        return instance;
    }

    JWindowsApp::ptr JWindowsApp::getInstance(int width, int height, const std::string title) {
        if(instance == nullptr) {
            instance = std::shared_ptr<JWindowsApp>(new JWindowsApp());
            if(!instance -> setup(width, height, title))
                return nullptr;
        }
        return instance;
    }

    JWindowsApp::LTimer::LTimer() {
        startTicks = 0;
        pausedTicks = 0;

        paused = false;
        started = false;
    }

    void JWindowsApp::LTimer::start() {
        started = true;
        paused = false;
        startTicks = SDL_GetTicks();
        pausedTicks = 0;
    }

    void JWindowsApp::LTimer::stop() {
        started = false;
        paused = false;
        startTicks = 0;
        pausedTicks = 0;
    }

    void JWindowsApp::LTimer::pause() {
        if(started && !paused) {
            paused = true;
            pausedTicks = SDL_GetTicks() - startTicks;
            startTicks = 0;
        }
    }

    void JWindowsApp::LTimer::play() {
        if(started && paused) {
            paused = false;
            startTicks = SDL_GetTicks() - pausedTicks;
            pausedTicks = 0;
        }
    }

    Uint32 JWindowsApp::LTimer::getTicks() {
        Uint32 time = 0;
        if(started) {
            if(paused)
                time = pausedTicks;
            else
                time = SDL_GetTicks() - startTicks;
        }
        return time;
    }

}
