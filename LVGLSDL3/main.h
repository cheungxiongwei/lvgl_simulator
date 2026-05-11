#pragma once

#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include <lvgl.h>
#include <lvgl/src/display/lv_display_private.h>

class LVGLSDL3
{
public:
    LVGLSDL3(int hor_res = 640, int ver_res = 480)
        : m_logical_width(hor_res),
          m_logical_height(ver_res),
          m_dpi_scale(1.0f),
          m_pixel_width(static_cast<int>(std::lround(m_logical_width * m_dpi_scale))),
          m_pixel_height(static_cast<int>(std::lround(m_logical_height * m_dpi_scale))) {
        // Initialize SDL
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_DisplayID instance_id = SDL_GetPrimaryDisplay();
        m_dpi_scale               = SDL_GetDisplayContentScale(instance_id);
        if (m_dpi_scale < 1.0) {
            m_dpi_scale = 1.0;
        }

        m_pixel_width  = static_cast<int>(std::lround(m_logical_width * m_dpi_scale));
        m_pixel_height = static_cast<int>(std::lround(m_logical_height * m_dpi_scale));

        // Create window
        SDL_WindowFlags flags = 0;
        flags |= SDL_WINDOW_OPENGL;
        flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
        flags |= SDL_WINDOW_BORDERLESS;     // 无边框
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;  // 置顶
        flags |= SDL_WINDOW_RESIZABLE;
        m_window = SDL_CreateWindow("LVGL with SDL3", m_pixel_width, m_pixel_height, flags);
        if (!m_window) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        if (!SDL_SetWindowSize(m_window, m_pixel_width, m_pixel_height)) {
            std::cerr << "SDL_SetWindowSize Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            return;
        }

        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        // Create renderer
        m_renderer = SDL_CreateRenderer(m_window, nullptr);
        if (!m_renderer) {
            std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            return;
        }

        // Create texture for LVGL rendering
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, m_logical_width, m_logical_height);
        if (!m_texture) {
            std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
            SDL_DestroyRenderer(m_renderer);
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            return;
        }

        // [1] Initialize LVGL library (must be called first)
        lv_init();

        // [2] Set up LVGL tick system
        lv_tick_set_cb(
            []() -> uint32_t
            {
                auto now = std::chrono::steady_clock::now();
                auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_system_start).count();
                return static_cast<uint32_t>(ms);
            });

        // [3] Initialize LVGL display
        // Allocate buffer for LVGL rendering
        uint32_t stride = lv_draw_buf_width_to_stride(m_logical_width, LV_COLOR_FORMAT_NATIVE);
        m_buffer_size   = stride * m_logical_height;  // RGBA32 = 4 bytes per pixel
        m_draw_buffer.resize(m_buffer_size);

        m_display = lv_display_create(m_logical_width, m_logical_height);
        lv_display_set_user_data(m_display, this);
        lv_display_set_antialiasing(m_display, true);
        lv_display_set_buffers(m_display, m_draw_buffer.data(), nullptr, m_buffer_size, LV_DISPLAY_RENDER_MODE_FULL);
        lv_display_set_flush_cb(m_display,
                                [](lv_display_t* disp, lv_area_t const* area, uint8_t* px_map)
                                {
                                    auto self = static_cast<LVGLSDL3*>(lv_display_get_user_data(disp));

                                    // Update the texture with the rendered content
                                    uint32_t stride = lv_draw_buf_width_to_stride(self->m_logical_width, LV_COLOR_FORMAT_NATIVE);
                                    SDL_UpdateTexture(self->m_texture, nullptr, px_map, stride);

                                    // Render the texture to screen
                                    SDL_RenderClear(self->m_renderer);
                                    SDL_RenderTexture(self->m_renderer, self->m_texture, nullptr, nullptr);
                                    SDL_RenderPresent(self->m_renderer);

                                    // Notify LVGL that flushing is done
                                    lv_display_flush_ready(disp);
                                });

        // [4] Initialize input devices
        // Mouse
        m_mouse_indev = lv_indev_create();
        lv_indev_set_user_data(m_mouse_indev, this);
        lv_indev_set_type(m_mouse_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(m_mouse_indev,
                             [](lv_indev_t* indev, lv_indev_data_t* data)
                             {
                                 auto self   = static_cast<LVGLSDL3*>(lv_indev_get_user_data(indev));
                                 data->state = self->m_mouse_state;
                                 data->point = self->m_mouse_point;
                             });

        // Keyboard
        m_key_indev = lv_indev_create();
        lv_indev_set_user_data(m_key_indev, this);
        lv_indev_set_type(m_key_indev, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(m_key_indev,
                             [](lv_indev_t* indev, lv_indev_data_t* data)
                             {
                                 auto self   = static_cast<LVGLSDL3*>(lv_indev_get_user_data(indev));
                                 data->state = self->m_key_state;
                                 data->key   = self->m_key_code;
                             });

        // Group
        lv_group_set_default(lv_group_create());
        lv_indev_set_group(m_mouse_indev, lv_group_get_default());
        lv_indev_set_group(m_key_indev, lv_group_get_default());

        lv_indev_set_display(m_mouse_indev, m_display);
        lv_indev_set_display(m_key_indev, m_display);

        lv_display_set_default(m_display);
    }

    ~LVGLSDL3() {
        if (m_texture)
            SDL_DestroyTexture(m_texture);
        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);
        if (m_window)
            SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void run() {
        if (!m_window) {
            return;
        }

        SDL_ShowWindow(m_window);
        // SDL_SetWindowAlwaysOnTop(m_window, true);

        m_quit = false;
        SDL_Event event;
        uint32_t lastTick           = SDL_GetTicks();
        uint32_t const tickInterval = 16;  // ~60 FPS

        while (!m_quit) {
            // Handle SDL events
            while (SDL_PollEvent(&event)) {
                processEvent(event);
            }

            // Update LVGL at regular intervals
            uint32_t currentTick = SDL_GetTicks();
            if (currentTick - lastTick >= tickInterval) {
                lv_timer_handler();
                lastTick = currentTick;
            }

            // Delay for remaining time in this frame
            // int64_t remaining = tickInterval - (SDL_GetTicks() - lastTick);
            // if (remaining > 0) {
            //     SDL_DelayPrecise(SDL_MS_TO_NS(remaining));
            // }
        }

        lv_deinit();
    }

    void stop() { m_quit = true; }

    // Getter for access to the LVGL display
    lv_display_t* getDisplay() const { return m_display; }
private:
    void processEvent(SDL_Event const& event) {
        SDL_Keycode keycode;
        switch (event.type) {
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                break;
            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
                break;
            case SDL_EVENT_QUIT:
                m_quit = true;
                break;

            case SDL_EVENT_MOUSE_MOTION:
                m_mouse_point.x = static_cast<int>(std::lround(event.motion.x / m_dpi_scale));
                m_mouse_point.y = static_cast<int>(std::lround(event.motion.y / m_dpi_scale));
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_mouse_state   = LV_INDEV_STATE_PRESSED;
                    m_mouse_point.x = static_cast<int>(std::lround(event.button.x / m_dpi_scale));
                    m_mouse_point.y = static_cast<int>(std::lround(event.button.y / m_dpi_scale));
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_mouse_state   = LV_INDEV_STATE_RELEASED;
                    m_mouse_point.x = static_cast<int>(std::lround(event.button.x / m_dpi_scale));
                    m_mouse_point.y = static_cast<int>(std::lround(event.button.y / m_dpi_scale));
                }
                break;

            case SDL_EVENT_KEY_DOWN:
                m_key_state = LV_INDEV_STATE_PRESSED;
                // https://wiki.libsdl.org/SDL3/BestKeyboardPractices
                keycode    = SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, false);
                m_key_code = convertSDLKeyToLVGL(keycode);
                break;

            case SDL_EVENT_KEY_UP:
                m_key_state = LV_INDEV_STATE_RELEASED;
                keycode     = SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, false);
                m_key_code  = convertSDLKeyToLVGL(keycode);
                break;
        }
    }

    uint32_t convertSDLKeyToLVGL(SDL_Keycode key) {
        // Map SDL key codes to LVGL key codes
        switch (key) {
            case SDLK_UP:
                return LV_KEY_UP;
            case SDLK_DOWN:
                return LV_KEY_DOWN;
            case SDLK_RIGHT:
                return LV_KEY_RIGHT;
            case SDLK_LEFT:
                return LV_KEY_LEFT;
            case SDLK_ESCAPE:
                return LV_KEY_ESC;
            case SDLK_DELETE:
                return LV_KEY_DEL;
            case SDLK_BACKSPACE:
                return LV_KEY_BACKSPACE;
            case SDLK_RETURN:
                return LV_KEY_ENTER;
            case SDLK_HOME:
                return LV_KEY_HOME;
            case SDLK_END:
                return LV_KEY_END;
            case SDLK_NUMLOCKCLEAR:
                return 144;
            case SDLK_KP_DIVIDE:
                return '/';
            case SDLK_KP_MULTIPLY:
                return '*';
            case SDLK_KP_MINUS:
                return '-';
            case SDLK_KP_PLUS:
                return '+';
            case SDLK_KP_ENTER:
                return 13;
            case SDLK_KP_1:
                return '1';
            case SDLK_KP_2:
                return '2';
            case SDLK_KP_3:
                return '3';
            case SDLK_KP_4:
                return '4';
            case SDLK_KP_5:
                return '5';
            case SDLK_KP_6:
                return '6';
            case SDLK_KP_7:
                return '7';
            case SDLK_KP_8:
                return '8';
            case SDLK_KP_9:
                return '9';
            case SDLK_KP_0:
                return '0';
            case SDLK_KP_PERIOD:
                return '.';
            case SDLK_KP_EQUALS:
                return '=';

            default:
                return key;  // Pass through other keys
        }
    }

    // Window dimensions
    int m_logical_width;
    int m_logical_height;

    float m_dpi_scale;

    int m_pixel_width;
    int m_pixel_height;

    // System time tracking
    inline static std::chrono::steady_clock::time_point m_system_start = std::chrono::steady_clock::now();

    // Input state
    lv_indev_state_t m_mouse_state = LV_INDEV_STATE_RELEASED;
    lv_point_t m_mouse_point       = {0, 0};
    lv_indev_state_t m_key_state   = LV_INDEV_STATE_RELEASED;
    uint32_t m_key_code            = 0;

    // SDL components
    SDL_Window* m_window     = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture   = nullptr;

    // LVGL components
    lv_display_t* m_display   = nullptr;
    lv_indev_t* m_mouse_indev = nullptr;
    lv_indev_t* m_key_indev   = nullptr;

    // Buffer for LVGL rendering
    std::vector<uint8_t> m_draw_buffer;
    size_t m_buffer_size = 0;

    // Main loop control
    bool m_quit = false;
};
