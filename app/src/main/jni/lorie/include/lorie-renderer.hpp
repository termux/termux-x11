#pragma once
#include <GLES2/gl2.h>
#include <limits.h>

class LorieCompositor;
class LorieRenderer {
public:
	LorieRenderer(LorieCompositor& compositor);
	void requestRedraw();
	void init();
	
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
	
	struct wl_event_source *idle = NULL;
	void drawCursor();
	void redraw();
    GLuint gTextureProgram = 0;
    GLuint gvPos = 0;
    GLuint gvCoords = 0;
    GLuint gvTextureSamplerHandle = 0;
    
    friend class LorieTexture;
};

class LorieTexture {
private:
	LorieRenderer* r = nullptr;
public:
	LorieTexture();
	uint32_t width, height;
	void uninit();
	void reinit(LorieRenderer* renderer, uint32_t _width, uint32_t _height);
	bool valid();
	void upload(void *data);
private:
	GLuint id = UINT_MAX;
	void draw(float x0, float y0, float x1, float y1);
	
	friend class LorieRenderer;
};
