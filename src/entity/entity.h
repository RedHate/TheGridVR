/*
 * RedHate's 3d VR project April 2020
 */

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>


#define bit(a, n) (a >> n)

#ifdef _WITH_LIBGLES
#define glDepthRange(x, y) glDepthRangef(x, y)
#define glFrustum(xmin,xmax,ymin,ymax,znear,zfar) glFrustumf(xmin,xmax,ymin,ymax,znear,zfar)
#define glOrtho(a, b, c, d, e, f) glOrthof(a, b, c, d, e, f)
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

typedef unsigned long       u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

u32 WINDOW_WIDTH  = (864);
u32 WINDOW_HEIGHT = (((WINDOW_WIDTH / 16) * 9));

typedef struct uv{
    float   u,
            v;
}uv;

typedef struct rgba{
    float   r,
            g,
            b,
            a;
}rgba;

typedef struct vertex{
    float   x,
            y,
            z;
}vertex;

typedef struct vertex_tcnv{
    uv          t;
    rgba        c;
    vertex      n,
                v;
} vertex_tcnv;

typedef struct timer{
    uint32_t ms;
    uint32_t delay;
    uint32_t st;
} timer;

typedef struct material{
    vertex ka,
           kd,
           ks;
    float  ns;
    char  *name;
} material;

typedef struct texture{
    int  w,
         h;
    u32  s;
    unsigned char    *data __attribute__((aligned(16)));
}texture;

typedef struct sprite{
    u32  w,
         h,
         s;
    u8   data[12][((32*32)*4)] __attribute__((aligned(16)));
}sprite;

typedef struct boundbox{
    vertex min,
           max;
}boundbox;

class entity{

    public:

        vertex          p,          /** position */
                        r;          /** rotation */
        boundbox        b;          /** bounding box */
        u16             m,          /** gl draw mode */
                        f;          /** facing */
        timer           time;       /** timer */
        u32             s,          /** length of vertex array */
                        h,          /** height */
                        w,          /** width */
                        heading;    /** heading */
        u32             pix[12];    /** texture pointers to hold the sprite frames */
        material        mat;        /** lighting material */
        vertex_tcnv    *vbo;        /** vbo*/

        /** BOX COLLISION DETECTION */
        int  col(entity e, short f);

        /** CONSTANT GRAVITATIONAL FORCE */
        void grav();

        /** LIGHTING (lingering may be of use later) */
        void setup_light(vertex ka, vertex kd, vertex ks, float ns);
        void apply_light();

        /** TEXTURE */
        void load_texture(const char *file);
        void load_texture_buffer(u32 *v_buf, u32 width, u32 height, u16 pixelformat);
        void apply_texture();

        /** SCREEN MASKING */
        void draw2d(int x, int y);
        void draw2dfull();

        /** MODEL */
        void genbox(vertex size, rgba color, u32 bFlip, u32 mode);

        /** POSITION AND ROTATION */
        void posrot(vertex pos, vertex rot);

        /** MODEL */
        void drawvbo();
        void draw();

        /** SPRITES */
        void getspritetex(sprite tex);
        void drawsprite();

        /** DEBUG PRINT STUFF NOT SURE IF I WILL KEEP IT BUT HEY IT WORKS FOR NOW */
        void dbginit(u32 w, u32 h);
        void dbgclear(u32 color);
        void dbgprint(const char *msg,int x,int y,int fg_col,int bg_col, int selected_font, ...);

        int openframebuffer(const char *device);
        int updateframebuffer();
        int closeframebuffer();

        int grabXorg();
        int setupXorg();

};

int getmax_(int x, int max){
    if(x <= max) return x-1;
    else return max;
}

/** BOX COLLISION DETECTION */
int  entity::col(entity e, short f){

    int retval = 0;

    if(e.s != 0){
        if(f == GL_CCW){
            //top
            if(((p.y + b.max.y > e.p.y + e.b.max.y) && (p.y + b.min.y < e.p.y + e.b.max.y)) &&
               ((p.z + b.max.z > e.p.z + e.b.min.z) && (p.z + b.min.z < e.p.z + e.b.max.z)) &&
               ((p.x + b.max.x > e.p.x + e.b.min.x) && (p.x + b.min.x < e.p.x + e.b.max.x))){
                    p.y = e.p.y + e.b.max.y + b.max.y;
                    retval += 1;
                }
            //bottom
            if(((p.y + b.max.y > e.p.y + e.b.min.y) && (p.y + b.min.y < e.p.y + e.b.min.y)) &&
               ((p.z + b.max.z > e.p.z + e.b.min.z) && (p.z + b.min.z < e.p.z + e.b.max.z)) &&
               ((p.x + b.max.x > e.p.x + e.b.min.x) && (p.x + b.min.x < e.p.x + e.b.max.x))){
                    p.y = e.p.y + e.b.min.y + b.min.y;
                    retval += 60;
                }
            //front
            if(((p.z + b.max.z > e.p.z + e.b.max.z) && (p.z + b.min.z < e.p.z + e.b.max.z)) &&
               ((p.y + b.max.y > e.p.y + e.b.min.y) && (p.y + b.min.y < e.p.y + e.b.max.y)) &&
               ((p.x + b.max.x > e.p.x + e.b.min.x) && (p.x + b.min.x < e.p.x + e.b.max.x))){
                    p.z = e.p.z + e.b.max.z + b.max.z;
                    retval += 200;
                }
            //back
            if(((p.z + b.max.z > e.p.z + e.b.min.z) && (p.z + b.min.z < e.p.z + e.b.min.z)) &&
               ((p.y + b.max.y > e.p.y + e.b.min.y) && (p.y + b.min.y < e.p.y + e.b.max.y)) &&
               ((p.x + b.max.x > e.p.x + e.b.min.x) && (p.x + b.min.x < e.p.x + e.b.max.x))){
                    p.z = e.p.z + e.b.min.z + b.min.z;
                    retval += 3000;
                }
            //right
            if(((p.x + b.max.x > e.p.x + e.b.max.x) && (p.x + b.min.x < e.p.x + e.b.max.x)) &&
               ((p.z + b.max.z > e.p.z + e.b.min.z) && (p.z + b.min.z < e.p.z + e.b.max.z)) &&
               ((p.y + b.max.y > e.p.y + e.b.min.y) && (p.y + b.min.y < e.p.y + e.b.max.y))){
                    p.x = e.p.x + e.b.max.x + b.max.x;
                    retval += 40000;
                }
            //left
            if(((p.x + b.max.x > e.p.x + e.b.min.x) && (p.x + b.min.x < e.p.x + e.b.min.x)) &&
               ((p.z + b.max.z > e.p.z + e.b.min.z) && (p.z + b.min.z < e.p.z + e.b.max.z)) &&
               ((p.y + b.max.y > e.p.y + e.b.min.y) && (p.y + b.min.y < e.p.y + e.b.max.y))){
                    p.x = e.p.x + e.b.min.x + b.min.x;
                    retval += 500000;
                }

        }
        else if(f == GL_CW){
            if(p.x > e.p.x + e.b.max.x - b.max.x){ p.x = e.p.x + e.b.max.x - b.max.x; retval++; }
            if(p.x < e.p.x + e.b.min.x - b.min.x){ p.x = e.p.x + e.b.min.x - b.min.x; retval++; }
            if(p.y > e.p.y + e.b.max.y - b.max.y){ p.y = e.p.y + e.b.max.y - b.max.y; retval++; }
            if(p.y < e.p.y + e.b.min.y - b.min.y){ p.y = e.p.y + e.b.min.y - b.min.y; retval++; }
            if(p.z > e.p.z + e.b.max.z - b.max.z){ p.z = e.p.z + e.b.max.z - b.max.z; retval++; }
            if(p.z < e.p.z + e.b.min.z - b.min.z){ p.z = e.p.z + e.b.min.z - b.min.z; retval++; }
        }
        else retval = 0;
    }

    return retval;

}

/** PHYSICS */
void entity::grav(){
    /* constant force gravity */
    p.y -= (0.98f * 0.98f);
}


/** LIGHTING MATERIAL */
void entity::setup_light(vertex ka, vertex kd, vertex ks, float ns){
    mat.ka=ka;
    mat.kd=kd;
    mat.ks=ks;
    mat.ns=ns;
    mat.name="generated mtl";
}

void entity::apply_light(){
    GLfloat attenuation = 0.075;
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, (GLfloat)attenuation);
    float lightPos[4]   =   { p.x, p.y, p.z, 1};
    glLightfv(GL_LIGHT1, GL_AMBIENT,   (GLfloat*)&mat.ka);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,   (GLfloat*)&mat.kd);
    glLightfv(GL_LIGHT1, GL_SPECULAR,  (GLfloat*)&mat.ks);
    glLightfv(GL_LIGHT1, GL_SHININESS, (GLfloat*)&mat.ns);
    glLightfv(GL_LIGHT1, GL_POSITION,  lightPos);
    glEnable(GL_LIGHT1);
}


/** TEXTURE */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

u16 getchannels(int channels){
    switch(channels)
    {
        case 3: return GL_RGB ; break;
        case 4: return GL_RGBA; break;
    }
    return 0;
}

void entity::load_texture(const char *file){

    int channels;
    u8 *data = stbi_load(file, (int*)&h, (int*)&w, &channels, 0);
    int pixelformat = getchannels(channels);
    printf("Texture found: %s %d %d %d\r\n", file, w, h, channels);
    glGenTextures(1, &pix[0]);
    glBindTexture(GL_TEXTURE_2D, pix[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, pixelformat, h, w, 0, pixelformat, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

}

void entity::load_texture_buffer(u32 *v_buf, u32 width, u32 height, u16 pixelformat){
    //LOL, YEAH... I know....
    w = width;
    h = height;
    glGenTextures(1, &pix[0]);
    glBindTexture(GL_TEXTURE_2D, pix[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, pixelformat, w, h, 0, pixelformat, GL_UNSIGNED_BYTE, v_buf);
}

void entity::apply_texture(){
    if((w || h) >= 0){
        if(pix[0] != 0){
            glFrontFace(f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, pix[0]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        }
    }
}


/** SCREEN MASKING */
void entity::draw2d(int x, int y){

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0, (float)WINDOW_WIDTH, 0, (float)WINDOW_HEIGHT, 0, 1.0);

    //glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);

    apply_texture();

    vertex_tcnv obj[6] = {
        //u  v  color        normx      normy      normz      vertx      verty      vertz
        //front face
        { 0, 1, {1,1,1,1}, 0,  0,  1,  x,   (WINDOW_HEIGHT)-y-h, 0 },
        { 1, 1, {1,1,1,1}, 0,  0,  1,  x+w, (WINDOW_HEIGHT)-y-h, 0 },
        { 0, 0, {1,1,1,1}, 0,  0,  1,  x,   (WINDOW_HEIGHT)-y,   0 },
        { 1, 1, {1,1,1,1}, 0,  0,  1,  x+w, (WINDOW_HEIGHT)-y-h, 0 },
        { 1, 0, {1,1,1,1}, 0,  0,  1,  x+w, (WINDOW_HEIGHT)-y,   0 },
        { 0, 0, {1,1,1,1}, 0,  0,  1,  x,   (WINDOW_HEIGHT)-y,   0 },
    };

    glEnable(GL_COLOR_MATERIAL);
    glPushMatrix();

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glTexCoordPointer(2, GL_FLOAT, 12*sizeof(float), &obj->t);
    glColorPointer(4, GL_FLOAT,  12*sizeof(float), &obj->c);
    glNormalPointer(GL_FLOAT,  12*sizeof(float), &obj->n);
    glVertexPointer(3, GL_FLOAT, 12*sizeof(float), &obj->v);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

}

void entity::draw2dfull(){

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0, (float)WINDOW_WIDTH, 0, (float)WINDOW_HEIGHT, 0, 1.0);

    //glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);

    apply_texture();

    vertex_tcnv obj[6] = {
        //u  v  color        normx      normy      normz      vertx      verty      vertz
        //front face
        { 0, 1, {1,1,1,1}, 0,  0,  1,  0,            0,                     0 },
        { 1, 1, {1,1,1,1}, 0,  0,  1,  WINDOW_WIDTH, 0,                     0 },
        { 0, 0, {1,1,1,1}, 0,  0,  1,  0,            WINDOW_HEIGHT,         0 },
        { 1, 1, {1,1,1,1}, 0,  0,  1,  WINDOW_WIDTH, 0,                     0 },
        { 1, 0, {1,1,1,1}, 0,  0,  1,  WINDOW_WIDTH, WINDOW_HEIGHT,         0 },
        { 0, 0, {1,1,1,1}, 0,  0,  1,  0,            WINDOW_HEIGHT,         0 },
    };

    glEnable(GL_COLOR_MATERIAL);
    glPushMatrix();

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glTexCoordPointer(2, GL_FLOAT, 12*sizeof(float), &obj->t);
    glColorPointer(4, GL_FLOAT,  12*sizeof(float), &obj->c);
    glNormalPointer(GL_FLOAT,  12*sizeof(float), &obj->n);
    glVertexPointer(3, GL_FLOAT, 12*sizeof(float), &obj->v);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

}

#define REPEAT    0
#define NO_REPEAT 1
/** CONSTRUCTORS */
void entity::genbox(vertex size, rgba color, u32 bFlip, u32 mode){

    int x=1,y=1,z=1;

    if(size.x > 1) x = (int)size.x;
    if(size.y > 1) y = (int)size.y;
    if(size.z > 1) z = (int)size.z;

    if(mode == NO_REPEAT){
        vertex_tcnv obj[36] = {
            //u  v  color        normx      normy      normz      vertx      verty      vertz
            //top face
            { 0, 1, {color.r, color.g, color.b, color.a}, 0,  1,  0, -size.x/2,  size.y/2,  size.z/2, },
            { 1, 0, {color.r, color.g, color.b, color.a}, 0,  1,  0,  size.x/2,  size.y/2, -size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a}, 0,  1,  0, -size.x/2,  size.y/2, -size.z/2, },
            { 0, 1, {color.r, color.g, color.b, color.a}, 0,  1,  0, -size.x/2,  size.y/2,  size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a}, 0,  1,  0,  size.x/2,  size.y/2,  size.z/2, },
            { 1, 0, {color.r, color.g, color.b, color.a}, 0,  1,  0,  size.x/2,  size.y/2, -size.z/2, },
            //front face
            { 0, 1, {color.r, color.g, color.b, color.a}, 0,  0,  1, -size.x/2, -size.y/2,  size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a}, 0,  0,  1,  size.x/2, -size.y/2,  size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0,  1, -size.x/2,  size.y/2,  size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a}, 0,  0,  1,  size.x/2, -size.y/2,  size.z/2, },
            { 1, 0, {color.r, color.g, color.b, color.a}, 0,  0,  1,  size.x/2,  size.y/2,  size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0,  1, -size.x/2,  size.y/2,  size.z/2, },
            //left face
            { 1, 0, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2,  size.y/2,  size.z/2, },
            { 0, 1, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2, -size.y/2, -size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2, -size.y/2,  size.z/2, },
            { 1, 0, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2,  size.y/2,  size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2,  size.y/2, -size.z/2, },
            { 0, 1, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2, -size.y/2, -size.z/2, },
            //back face
            { 0, 1, {color.r, color.g, color.b, color.a}, 0,  0, -1,  size.x/2, -size.y/2, -size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a}, 0,  0, -1, -size.x/2, -size.y/2, -size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0, -1,  size.x/2,  size.y/2, -size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a}, 0,  0, -1, -size.x/2, -size.y/2, -size.z/2, },
            { 1, 0, {color.r, color.g, color.b, color.a}, 0,  0, -1, -size.x/2,  size.y/2, -size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0, -1,  size.x/2,  size.y/2, -size.z/2, },
            //right face
            { 1, 0, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2,  size.y/2, -size.z/2, },
            { 0, 1, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2, -size.y/2,  size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2, -size.y/2, -size.z/2, },
            { 1, 0, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2,  size.y/2, -size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2,  size.y/2,  size.z/2, },
            { 0, 1, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2, -size.y/2,  size.z/2, },
            //bottom face
            { 1, 0, {color.r, color.g, color.b, color.a}, 0, -1,  0,  size.x/2, -size.y/2,  size.z/2, },
            { 0, 1, {color.r, color.g, color.b, color.a}, 0, -1,  0, -size.x/2, -size.y/2, -size.z/2, },
            { 1, 1, {color.r, color.g, color.b, color.a}, 0, -1,  0,  size.x/2, -size.y/2, -size.z/2, },
            { 1, 0, {color.r, color.g, color.b, color.a}, 0, -1,  0,  size.x/2, -size.y/2,  size.z/2, },
            { 0, 0, {color.r, color.g, color.b, color.a}, 0, -1,  0, -size.x/2, -size.y/2,  size.z/2, },
            { 0, 1, {color.r, color.g, color.b, color.a}, 0, -1,  0, -size.x/2, -size.y/2, -size.z/2, },
        };

        if(bFlip){
            u32 i;
            for(i=0;i<36;i++){
                     if(obj[i].n.x < 0)   obj[i].n.x =  1;
                else if(obj[i].n.x > 0)   obj[i].n.x = -1;
                     if(obj[i].n.y < 0)   obj[i].n.y =  1;
                else if(obj[i].n.y > 0)   obj[i].n.y = -1;
                     if(obj[i].n.z < 0)   obj[i].n.z =  1;
                else if(obj[i].n.z > 0)   obj[i].n.z = -1;
            }
        }

        vbo = (vertex_tcnv*) malloc(sizeof(vertex_tcnv)*36);
        memcpy(vbo, obj, sizeof(vertex_tcnv)*36);

        u32 i;
        for(i=0;i<36;i++){
            if(b.min.x > obj[i].v.x) b.min.x = obj[i].v.x;
            if(b.max.x < obj[i].v.x) b.max.x = obj[i].v.x;
            if(b.min.y > obj[i].v.y) b.min.y = obj[i].v.y;
            if(b.max.y < obj[i].v.y) b.max.y = obj[i].v.y;
            if(b.min.z > obj[i].v.z) b.min.z = obj[i].v.z;
            if(b.max.z < obj[i].v.z) b.max.z = obj[i].v.z;
        }
    }
    else
    {
        vertex_tcnv obj[36] = {
                //u  v  color        normx      normy      normz      vertx      verty      vertz
                //top face
                { 0, z, {color.r, color.g, color.b, color.a}, 0,  1,  0, -size.x/2,  size.y/2,  size.z/2, },
                { x, 0, {color.r, color.g, color.b, color.a}, 0,  1,  0,  size.x/2,  size.y/2, -size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a}, 0,  1,  0, -size.x/2,  size.y/2, -size.z/2, },
                { 0, z, {color.r, color.g, color.b, color.a}, 0,  1,  0, -size.x/2,  size.y/2,  size.z/2, },
                { x, z, {color.r, color.g, color.b, color.a}, 0,  1,  0,  size.x/2,  size.y/2,  size.z/2, },
                { x, 0, {color.r, color.g, color.b, color.a}, 0,  1,  0,  size.x/2,  size.y/2, -size.z/2, },
                //front face
                { 0, y, {color.r, color.g, color.b, color.a}, 0,  0,  1, -size.x/2, -size.y/2,  size.z/2, },
                { x, y, {color.r, color.g, color.b, color.a}, 0,  0,  1,  size.x/2, -size.y/2,  size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0,  1, -size.x/2,  size.y/2,  size.z/2, },
                { x, y, {color.r, color.g, color.b, color.a}, 0,  0,  1,  size.x/2, -size.y/2,  size.z/2, },
                { x, 0, {color.r, color.g, color.b, color.a}, 0,  0,  1,  size.x/2,  size.y/2,  size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0,  1, -size.x/2,  size.y/2,  size.z/2, },
                //left face
                { z, 0, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2,  size.y/2,  size.z/2, },
                { 0, y, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2, -size.y/2, -size.z/2, },
                { z, y, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2, -size.y/2,  size.z/2, },
                { z, 0, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2,  size.y/2,  size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2,  size.y/2, -size.z/2, },
                { 0, y, {color.r, color.g, color.b, color.a},-1,  0,  0, -size.x/2, -size.y/2, -size.z/2, },
                //back face
                { 0, y, {color.r, color.g, color.b, color.a}, 0,  0, -1,  size.x/2, -size.y/2, -size.z/2, },
                { x, y, {color.r, color.g, color.b, color.a}, 0,  0, -1, -size.x/2, -size.y/2, -size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0, -1,  size.x/2,  size.y/2, -size.z/2, },
                { x, y, {color.r, color.g, color.b, color.a}, 0,  0, -1, -size.x/2, -size.y/2, -size.z/2, },
                { x, 0, {color.r, color.g, color.b, color.a}, 0,  0, -1, -size.x/2,  size.y/2, -size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a}, 0,  0, -1,  size.x/2,  size.y/2, -size.z/2, },
                //right face
                { z, 0, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2,  size.y/2, -size.z/2, },
                { 0, y, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2, -size.y/2,  size.z/2, },
                { z, y, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2, -size.y/2, -size.z/2, },
                { z, 0, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2,  size.y/2, -size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2,  size.y/2,  size.z/2, },
                { 0, y, {color.r, color.g, color.b, color.a}, 1,  0,  0,  size.x/2, -size.y/2,  size.z/2, },
                //bottom face
                { x, 0, {color.r, color.g, color.b, color.a}, 0, -1,  0,  size.x/2, -size.y/2,  size.z/2, },
                { 0, z, {color.r, color.g, color.b, color.a}, 0, -1,  0, -size.x/2, -size.y/2, -size.z/2, },
                { x, z, {color.r, color.g, color.b, color.a}, 0, -1,  0,  size.x/2, -size.y/2, -size.z/2, },
                { x, 0, {color.r, color.g, color.b, color.a}, 0, -1,  0,  size.x/2, -size.y/2,  size.z/2, },
                { 0, 0, {color.r, color.g, color.b, color.a}, 0, -1,  0, -size.x/2, -size.y/2,  size.z/2, },
                { 0, z, {color.r, color.g, color.b, color.a}, 0, -1,  0, -size.x/2, -size.y/2, -size.z/2, },
            };

        if(bFlip){
            u32 i;
            for(i=0;i<36;i++){
                     if(obj[i].n.x < 0)   obj[i].n.x =  1;
                else if(obj[i].n.x > 0)   obj[i].n.x = -1;
                     if(obj[i].n.y < 0)   obj[i].n.y =  1;
                else if(obj[i].n.y > 0)   obj[i].n.y = -1;
                     if(obj[i].n.z < 0)   obj[i].n.z =  1;
                else if(obj[i].n.z > 0)   obj[i].n.z = -1;
            }
        }

        vbo = (vertex_tcnv*) malloc(sizeof(vertex_tcnv)*36);
        memcpy(vbo, obj, sizeof(vertex_tcnv)*36);

        u32 i;
        for(i=0;i<36;i++){
            if(b.min.x > obj[i].v.x) b.min.x = obj[i].v.x;
            if(b.max.x < obj[i].v.x) b.max.x = obj[i].v.x;
            if(b.min.y > obj[i].v.y) b.min.y = obj[i].v.y;
            if(b.max.y < obj[i].v.y) b.max.y = obj[i].v.y;
            if(b.min.z > obj[i].v.z) b.min.z = obj[i].v.z;
            if(b.max.z < obj[i].v.z) b.max.z = obj[i].v.z;
        }
    }



    f = GL_CCW;
    s = 36;
    m = GL_TRIANGLES;

}

/** POSITION AND ROTATION */
void entity::posrot(vertex pos, vertex rot){
    p = pos;
    r = rot;
};


/** MODEL */
void entity::drawvbo(){

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    {
        glTranslatef(p.x, p.y, p.z);
        glRotatef(-r.x, 0, 0, 1);
        glRotatef(-r.y, 0, 1, 0);
        glRotatef(-r.z, 1, 0, 0);
    }

    glEnable(GL_COLOR_MATERIAL);
    glPushMatrix();

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glTexCoordPointer(2, GL_FLOAT, 12*sizeof(float), &vbo->t);
    glColorPointer(4, GL_FLOAT,  12*sizeof(float), &vbo->c);
    glNormalPointer(GL_FLOAT,  12*sizeof(float), &vbo->n);
    glVertexPointer(3, GL_FLOAT, 12*sizeof(float), &vbo->v);
    glDrawArrays(m, 0, s);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHT1);

    glLoadIdentity();

}

void entity::draw(){
    apply_light();
    apply_texture();
    drawvbo();
}


/** SPRITES */
int animating_frame = 0;

void entity::getspritetex(sprite tex){

    w = tex.w;
    h = tex.h;
    glGenTextures(12, pix);
    int i;
    for(i=0;i<12;i++){
        glBindTexture(GL_TEXTURE_2D, pix[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.w, tex.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &tex.data[i][0]);
    }

}

void entity::drawsprite(){

    apply_light(); //doesnt do shit when the light when not initialised
    if((w || h) >= 1){
        time.ms = SDL_GetTicks();
        if((time.ms - time.st) > 1000.0f/4){
            time.st = time.ms;
            if(animating_frame<2) animating_frame++;
            else animating_frame = 1;
        }
        if(pix[heading*3+animating_frame] != 0){
            glFrontFace(f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, pix[heading*3+animating_frame]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        }
    }
    drawvbo();

}

#include "debugfonts.h"

/** DEBUG */
u32 DEBUG_WIDTH     = 0; //720
u32 DEBUG_HEIGHT    = 0; //+50 to account for spacing from the top menu
u32 *vbuffer __attribute__((aligned(16))); /** framebuffer */

void entity::dbginit(u32 w, u32 h){
    DEBUG_WIDTH = w;
    DEBUG_HEIGHT = h;
    u32 *internalbuffer = (u32*) malloc((w * sizeof(u32)) * h);
    //u32 internalbuffer[DEBUG_WIDTH*DEBUG_HEIGHT] __attribute__((aligned(16))); /** framebuffer */
    vbuffer=internalbuffer;
}

void entity::dbgclear(u32 color){
    u32 *scr = (u32*)vbuffer;
    u32 i; for(i=0;i<DEBUG_WIDTH*DEBUG_HEIGHT;i++) scr[i] = color;
}

void entity::dbgprint(const char *msg,int x,int y,int fg_col,int bg_col, int selected_font, ...){

    char buf[256];
    va_list ap;
    va_start(ap, msg);
    vsnprintf(buf, 256, msg, ap);
    va_end(ap);
    buf[255] = 0;

    int a,b,p;
    int offset;
    char code;

    y = y * 7; //if set to 8 it creates a 1px space between rows, i assume due to 0 counted as interger in the loops thus making it 8.

    u8 font;
    u32 *scr            = (u32*)vbuffer;
    u32 pwidth          = sizeof(u32) * DEBUG_WIDTH;
    u32 bufferwidth     = DEBUG_WIDTH;

    for(a=0;buf[a] && a<(pwidth/8);a++){
        code = buf[a] & 0x7f; // 7bit ANK
        for(b=0;b<7;b++){
            offset = (y+b)*bufferwidth + (x+a)*8;
            font = msx[selected_font][ code*8 + b ];
            for(p=0;p<8;p++){
                scr[offset] = (font & 0x80) ? fg_col : bg_col;
                font <<= 1;
                offset++;
            }
        }
    }

    w = DEBUG_WIDTH;
    h = DEBUG_HEIGHT;

    glGenTextures(1, &pix[0]);
    glBindTexture(GL_TEXTURE_2D, pix[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DEBUG_WIDTH, DEBUG_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, scr);

}

//create an internal buffer for the framebuffer flip function
u8 __attribute__((aligned(16)))  internalbuffer[1366*768*4];

int fbfd = 0;
u8 *fbp = 0;
long int screensize = 0;

int entity::openframebuffer(const char *device){

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    fbfd = open(device, O_RDWR);
    if (fbfd == -1){
        printf("Error cannot open device.\r\n");
        return 0;
    }

    printf("The device was opened successfully.\r\n");

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1){
        printf("Error reading fixed information.\r\n");
        return 0;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1){
        printf("Error reading variable information.\r\n");
        return 0;
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    //screensize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);

    screensize = 1376 * 768 * 4;

    printf("Screensize: %d\r\n", screensize);

    fbp = (u8*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (*fbp == -1){
        printf("Error failed to map framebuffer device to memory.\r\n");
        return 0;
    }

    //debug stuff
    printf("The device was mapped to memory successfully.\r\n");
    printf(" Pixel format : RGBX_%ld%ld%ld%ld\n",vinfo.red.length, vinfo.green.length, vinfo.blue.length, vinfo.transp.length);

    //w = vinfo.xres;
    //h = vinfo.yres;

    //testing resolutions to find a fit but nothing working.
    //set gfxmode=1920x1080,1680x1050,1600x900,1366x768,1280x850,1024x768,auto
    w = 1376;
    h = 768;

    //more debug
    printf("w %d, h %d, xres %d\r\n, yres %d\r\n", w, h, vinfo.xres, vinfo.yres);

}

int entity::updateframebuffer(){
    glGenTextures(1, &pix[0]);
    glBindTexture(GL_TEXTURE_2D, pix[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED | GL_GREEN | GL_BLUE | NULL, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, fbp);
}

int entity::closeframebuffer(){
    munmap(fbp, screensize);
    close(fbfd);
}

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>

XWindowAttributes gwa;
Display *display = NULL;
Window my_window = NULL;

int entity::grabXorg(){

    XImage *image =  XGetImage(display, my_window, 0, 0, w, h, AllPlanes, ZPixmap);
    glBindTexture(GL_TEXTURE_2D, pix[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED | GL_GREEN | GL_BLUE | NULL, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, image->data);
    XDestroyImage(image);

}

int entity::setupXorg(){

    display = XOpenDisplay(NULL);
    if(display == NULL){ printf("Failed to connect to display, are you using xorg?\r\n"); exit(0); }

    my_window = DefaultRootWindow(display);
    if(my_window == NULL){ printf("Failed to connect to window, are you using xorg?\r\n"); exit(0); }

    XGetWindowAttributes(display, my_window, &gwa);
    w = gwa.width;
    h = gwa.height;

    glGenTextures(1, &pix[0]);

}

