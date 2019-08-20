#pragma once
#include <GLES2/gl2.h>
#include <limits.h>
#include "LorieImpls.hpp"

class LorieCompositor;
class LorieSurface;
class LorieRenderer {
public:
	LorieRenderer(LorieCompositor& compositor);
	void requestRedraw();
	void init();
	void uninit();
	
	uint32_t width = 1024;
	uint32_t height = 600;
	uint32_t physical_width = 270;
	uint32_t physical_height = 158;
	
	bool cursorVisible = false;
	
	uint32_t hotspot_x, hotspot_y;
	
	void resize(uint32_t w, uint32_t h, uint32_t pw, uint32_t ph); 
	void cursorMove(uint32_t x, uint32_t y);
	void setCursorVisibility(bool visibility);
	~LorieRenderer();
private:
	static void idleDraw(void *data);
	LorieCompositor& compositor;

	void set_toplevel(LorieSurface *surface);
	void set_cursor(LorieSurface *surface, uint32_t hotspot_x, uint32_t hotspot_y);

	LorieSurface* toplevel_surface = nullptr;
	LorieSurface* cursor_surface = nullptr;
	
	struct wl_event_source *idle = NULL;
	void drawCursor();
	void redraw();
    GLuint gTextureProgram = 0;
    GLuint gvPos = 0;
    GLuint gvCoords = 0;
    GLuint gvTextureSamplerHandle = 0;
    bool ready = false;
    
    friend class LorieTexture;
    friend class LorieCompositor;
};

class LorieTexture {
private:
	LorieRenderer* r = nullptr;
	bool damaged = false;
public:
	LorieTexture();
	uint32_t width, height;
	void *data = nullptr;
	void set_data(LorieRenderer* renderer, uint32_t width, uint32_t height, void *data);
	void damage(int32_t x, int32_t y, int32_t width, int32_t height);
	void uninit();
	void reinit();
	bool valid();
private:
	GLuint id = UINT_MAX;
	void draw(float x0, float y0, float x1, float y1);
	
	friend class LorieRenderer;
};
