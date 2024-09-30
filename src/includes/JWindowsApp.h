//
// Created by jonas on 2024/9/29.
//


#ifndef JWINDOWSAPP_H
#define JWINDOWSAPP_H

#include "SDL2/SDL.h"
#include "JParallelWrapper.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>
using std::string;
using uint = unsigned int;
using uchar = unsigned char;

namespace JackalRenderer {
    class JWindowsApp final {
    private:
        JWindowsApp() = default;
        JWindowsApp(JWindowsApp&) = delete;
        JWindowsApp& operator=(const JWindowsApp&) = delete;

        bool setup(int width, int height, string title);

    public:
        using ptr = std::shared_ptr<JWindowsApp>;

        class LTimer {
        public:
            LTimer();
            void start();
            void stop();
            void pause();
            void play();

            Uint32 getTicks();

            bool isStarted() { return started; }
            bool isPaused() { return paused && started; }

        private:
            Uint32 startTicks, pausedTicks;
            bool paused, started;
        };

        ~JWindowsApp();
        void readytoStart();

        void processEvent();
        bool shouldWindowClose() const { return quit; }
        double getTimeFromStart() { return timer.getTicks(); }
        int getMouseMotionDeltaX() const { return mouseDeltaX; }
        int getMouseMotionDeltaY() const { return mouseDeltaY; }
        int getMouseWheelDelta() const { return wheelDelta; }
        bool getISMouseLeftButtonPressed() const { return mouseLeftButtonPressed; }

        double updateScreenSurface(uchar* pixels, int width, int height, int channel, uint num_triangles);

        static JWindowsApp::ptr getInstance();
        static JWindowsApp::ptr getInstance(int width, int height, const std::string title = "winApp");

    private:
        int lastMouseX, lastMouseY;
        int mouseDeltaX, mouseDeltaY;
        bool mouseLeftButtonPressed = false;
        int lastWheelPos;
        int wheelDelta;

        LTimer timer;
        double lastTimePoint;
        double deltaTime;
        double fpsCounter;
        double fpsTimeRecorder;
        uint fps;

        int screenWidth, screenHeight;

        bool quit = false;

        string windowTitle;
        SDL_Event events;

        SDL_Window* windowHandle = nullptr;
        SDL_Surface* screenSurface = nullptr;

        static JWindowsApp::ptr instance;
    };
}

#endif //JWINDOWSAPP_H
