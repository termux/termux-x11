#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h> // GLES error descriotions
#include <stdint.h>

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

struct renderer {
    GLuint gTextureProgram;
    GLuint gvPos;
    GLuint gvCoords;
    GLuint gvTextureSamplerHandle;
};

struct texture {
	GLuint id;
	int width, height;
	struct {
		int32_t x, y, width, height;
	} damage;
};

struct renderer *renderer_create(void);
void renderer_destroy(struct renderer** renderer);

struct texture *texture_create(int width, int height);
void texture_upload(struct texture *texture, void *data);
void texture_draw(struct renderer* renderer, struct texture *texture, float x0, float y0, float x1, float y1);
void texture_destroy(struct texture** texture);
