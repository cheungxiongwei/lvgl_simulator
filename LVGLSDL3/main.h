#pragma once

#include <chrono>
#include <SDL3/SDL.h>
#include <lvgl.h>
#include <lvgl/src/display/lv_display_private.h>
#include <iostream>

class LVGLSDL3
{
public:
    LVGLSDL3(int hor_res = 640, int ver_res = 480)
      : width(hor_res)
      , height(ver_res)
      , mouse_state(LV_INDEV_STATE_RELEASED)
      , key_state(LV_INDEV_STATE_RELEASED)
      , key_code(0)
      , window(nullptr)
      , renderer(nullptr)
      , texture(nullptr)
      , display(nullptr)
      , mouse_indev(nullptr)
      , key_indev(nullptr)
      , quit(false) {
        // Initialize SDL
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Create window
        window = SDL_CreateWindow("LVGL with SDL3", width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS /*无边框*/ | SDL_WINDOW_ALWAYS_ON_TOP /*置顶*/);
        if(!window) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        // Create renderer
        renderer = SDL_CreateRenderer(window, nullptr);
        if(!renderer) {
            std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return;
        }

        // Create texture for LVGL rendering
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, width, height);
        if(!texture) {
            std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return;
        }

        // [1] Initialize LVGL library (must be called first)
        lv_init();

        // [2] Set up LVGL tick system
        lv_tick_set_cb([]() -> uint32_t {
            auto now = std::chrono::steady_clock::now();
            auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now - system_start).count();
            return static_cast<uint32_t>(ms);
        });

        // [3] Initialize LVGL display
        // Allocate buffer for LVGL rendering
        buffer_size = width * height * 4;  // RGBA32 = 4 bytes per pixel
        draw_buffer.resize(buffer_size);

        display = lv_display_create(width, height);
        lv_display_set_user_data(display, this);
        lv_display_set_antialiasing(display, true);
        lv_display_set_buffers(display, draw_buffer.data(), nullptr, buffer_size, LV_DISPLAY_RENDER_MODE_FULL);
        lv_display_set_flush_cb(display, [](lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
            auto self = static_cast<LVGLSDL3*>(lv_display_get_user_data(disp));

            // Update the texture with the rendered content
            SDL_UpdateTexture(self->texture, nullptr, px_map, self->width * 4);

            // Render the texture to screen
            SDL_RenderClear(self->renderer);
            SDL_RenderTexture(self->renderer, self->texture, nullptr, nullptr);
            SDL_RenderPresent(self->renderer);

            // Notify LVGL that flushing is done
            lv_display_flush_ready(disp);
        });

        // [4] Initialize input devices
        // Mouse
        mouse_indev = lv_indev_create();
        lv_indev_set_user_data(mouse_indev, this);
        lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(mouse_indev, [](lv_indev_t* indev, lv_indev_data_t* data) {
            auto self   = static_cast<LVGLSDL3*>(lv_indev_get_user_data(indev));
            data->state = self->mouse_state;
            data->point = self->mouse_point;
        });

        // Keyboard
        key_indev = lv_indev_create();
        lv_indev_set_user_data(key_indev, this);
        lv_indev_set_type(key_indev, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(key_indev, [](lv_indev_t* indev, lv_indev_data_t* data) {
            auto self   = static_cast<LVGLSDL3*>(lv_indev_get_user_data(indev));
            data->state = self->key_state;
            data->key   = self->key_code;
        });
    }

    ~LVGLSDL3() {
        if(texture) SDL_DestroyTexture(texture);
        if(renderer) SDL_DestroyRenderer(renderer);
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void run() {
        quit = false;
        SDL_Event event;
        uint32_t lastTick           = SDL_GetTicks();
        const uint32_t tickInterval = 16;  // ~60 FPS

        while(!quit) {
            // Handle SDL events
            while(SDL_PollEvent(&event)) {
                processEvent(event);
            }

            // Update LVGL at regular intervals
            uint32_t currentTick = SDL_GetTicks();
            if(currentTick - lastTick >= tickInterval) {
                lv_timer_handler();
                lastTick = currentTick;
            }
        }
    }

    void stop() { quit = true; }

    // Getter for access to the LVGL display
    lv_display_t* getDisplay() const { return display; }

private:
    void processEvent(const SDL_Event& event) {
        switch(event.type) {
            case SDL_EVENT_QUIT: quit = true; break;

            case SDL_EVENT_MOUSE_MOTION:
                mouse_point.x = event.motion.x;
                mouse_point.y = event.motion.y;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if(event.button.button == SDL_BUTTON_LEFT) {
                    mouse_state   = LV_INDEV_STATE_PRESSED;
                    mouse_point.x = event.button.x;
                    mouse_point.y = event.button.y;
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if(event.button.button == SDL_BUTTON_LEFT) {
                    mouse_state   = LV_INDEV_STATE_RELEASED;
                    mouse_point.x = event.button.x;
                    mouse_point.y = event.button.y;
                }
                break;

            case SDL_EVENT_KEY_DOWN:
                key_state = LV_INDEV_STATE_PRESSED;
                key_code  = convertSDLKeyToLVGL(event.key.key);
                break;

            case SDL_EVENT_KEY_UP:
                key_state = LV_INDEV_STATE_RELEASED;
                key_code  = convertSDLKeyToLVGL(event.key.key);
                break;
        }
    }

    uint32_t convertSDLKeyToLVGL(SDL_Keycode key) {
        // Map SDL key codes to LVGL key codes
        switch(key) {
            case SDLK_UP: return LV_KEY_UP;
            case SDLK_DOWN: return LV_KEY_DOWN;
            case SDLK_RIGHT: return LV_KEY_RIGHT;
            case SDLK_LEFT: return LV_KEY_LEFT;
            case SDLK_ESCAPE: return LV_KEY_ESC;
            case SDLK_DELETE: return LV_KEY_DEL;
            case SDLK_BACKSPACE: return LV_KEY_BACKSPACE;
            case SDLK_RETURN: return LV_KEY_ENTER;
            case SDLK_HOME: return LV_KEY_HOME;
            case SDLK_END: return LV_KEY_END;
            default: return key;  // Pass through other keys
        }
    }

    // Window dimensions
    int width, height;

    // System time tracking
    static inline std::chrono::steady_clock::time_point system_start = std::chrono::steady_clock::now();

    // Input state
    lv_indev_state_t mouse_state;
    lv_point_t mouse_point;
    lv_indev_state_t key_state;
    uint32_t key_code;

    // SDL components
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    // LVGL components
    lv_display_t* display;
    lv_indev_t* mouse_indev;
    lv_indev_t* key_indev;

    // Buffer for LVGL rendering
    std::vector<uint8_t> draw_buffer;
    size_t buffer_size;

    // Main loop control
    bool quit;
};
