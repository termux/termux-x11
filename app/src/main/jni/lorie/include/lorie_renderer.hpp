#pragma once
#include <GLES2/gl2.h>
#include <limits.h>

class lorie_renderer;
class lorie_texture {
private:
	lorie_renderer* r = nullptr;
	bool damaged = false;
public:
	lorie_texture();
	int width{}, height{};
	void *data{};
	void set_data(lorie_renderer* renderer, uint32_t width, uint32_t height, void *data);
	void damage(int32_t x, int32_t y, int32_t width, int32_t height);
	void uninit();
	void reinit();
	bool valid();
private:
	GLuint id = UINT_MAX;
	void draw(float x0, float y0, float x1, float y1);

	friend class lorie_renderer;
};

class lorie_compositor;
class lorie_renderer {
public:
	lorie_renderer(lorie_compositor& compositor);
	void request_redraw();
	void on_surface_create();
	
	uint32_t width = 1024;
	uint32_t height = 600;
	uint32_t physical_width = 270;
	uint32_t physical_height = 158;
	
	bool cursor_visible = false;
	
	uint32_t hotspot_x{}, hotspot_y{};
	
	void resize(int w, int h, uint32_t pw, uint32_t ph);
	void cursor_move(uint32_t x, uint32_t y);
	void set_cursor_visibility(bool visibility);
	std::function<void(bool)> set_renderer_visibility = [](bool){};
private:
	lorie_compositor& compositor;

	void set_toplevel(wayland::surface_t* surface);
	void set_cursor(wayland::surface_t* surface, uint32_t hotspot_x, uint32_t hotspot_y);
	
	struct wl_event_source *idle = NULL;
	void draw(GLuint id, float x0, float y0, float x1, float y1) const;
	void draw_cursor();
	void redraw();
    GLuint g_texture_program = 0;
    GLuint gv_pos = 0;
    GLuint gv_coords = 0;
    GLuint gv_texture_sampler_handle = 0;

	struct {
		GLuint id;
		GLsizei width, height;
		inline void reset() { width = 0; height = 0; }
		inline bool valid() { return width != 0 && height != 0; }
	} toplevel_texture{}, cursor_texture{};

    bool ready = false;
	bool visible = false;
    
    friend class lorie_texture;
    friend class lorie_compositor;
};
