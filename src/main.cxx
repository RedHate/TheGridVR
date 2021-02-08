/*
 * RedHate's 3d VR project September 2020
 */

#ifdef _WITH_LIBGLES
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "entity/entity.h"
#include "libpsvr/psvr.h"
#include <X11/extensions/Xrandr.h>

#include "../textures/player_sprite.h"

//#define _WITH_GLES 1

#define WINDOW_TITLE            "A team of monkeys - The little engine that could."
#define PI                      3.141593f

#define MAX_ENTITIES    32*32
#define HEADSPEED       2.5f
#define PI              3.141593f
#define MAP_SIZE        16

entity ent[MAX_ENTITIES];

#define player      ent[0]
#define debug       ent[1]
#define camera      ent[2]
#define skybox      ent[3]
#define jumbotron   ent[4]
#define light       ent[6]
#define mapbox      ent[7]
#define ground      ent[8]

typedef struct mouse_dev{
    short prev_x;
    short prev_y;
}mouse_dev;

typedef struct psvr_dev{
    //init psvr
    int    init;
    struct psvr_context *ctx;
    struct psvr_sensor_frame a_frame;
    int    retval;
    short  previousx;
    short  previousy;
    short  previousz;
    float  focus;
    int    mode;
}psvr_dev;

static bool running = true;

bool IsKeyDown( SDL_Keycode k ){
    int numKeys;
    static const Uint8* kb_keys = NULL;
    kb_keys = SDL_GetKeyboardState( &numKeys );
    return kb_keys[SDL_GetScancodeFromKey( k )] ? true : false;
}

void error(const char *msg){
    perror(msg);
    exit(0);
}

static void Perspective(float yfov, float aspect, float znear, float zfar){
    float
    ymax = znear * tan( yfov * PI / 360.0f ),
    ymin = -ymax,
    xmin = ymin * aspect,
    xmax = ymax * aspect;
    glFrustum(xmin,xmax,ymin,ymax,znear,zfar); //glFrustumf
}

static void SetLight(vertex pos, material mat, GLenum pname){
    /**
        if set to player values must be inverse (probably due to me reversing controls or view or something in the circuit of things...)
    */
    GLfloat attenuation = 0.00025;
    glLightf(pname, GL_QUADRATIC_ATTENUATION, (GLfloat)attenuation);
    float lightPos[4]   =   { pos.x, pos.y, pos.z, 1};
    glLightfv(pname, GL_AMBIENT,   (GLfloat*)&mat.ka);
    glLightfv(pname, GL_DIFFUSE,   (GLfloat*)&mat.kd);
    glLightfv(pname, GL_SPECULAR,  (GLfloat*)&mat.ks);
    glLightfv(pname, GL_SHININESS, (GLfloat*)&mat.ns);
    glLightfv(pname, GL_POSITION,  lightPos);
    glEnable(pname);
}

static void DrawScene(){

    //light
    SetLight((vertex){  light.p.x,light.p.y-2,light.p.z },        (material) {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, 0, "1"}, GL_LIGHT0);

    //3d stuff
    mapbox.draw();
    ground.draw();
    //jumbotron.draw();
    light.draw();
    player.drawsprite();

    //2d stuff
    glDisable(GL_LIGHTING);
    //camera.drawtex((WINDOW_WIDTH/2)-(camera.w/2), (WINDOW_HEIGHT/2)-(camera.h/2));
    debug.draw2dfull();
    glEnable(GL_LIGHTING);

}

Display                 *dpy;
Window                  root;
int                     num_sizes;
XRRScreenSize           *xrrs;
XRRScreenConfiguration  *conf;
short                   possible_frequencies[64][64];
short                   original_rate;
Rotation                original_rotation;
SizeID                  original_size_id;

//working resolution control test but not implemented
void set_resolution(){

    // CONNECT TO X-SERVER, GET ROOT WINDOW ID
    dpy    = XOpenDisplay(NULL);
    root   = RootWindow(dpy, 0);

    // GET CURRENT RESOLUTION AND FREQUENCY
    conf                   = XRRGetScreenInfo(dpy, root);
    original_rate          = XRRConfigCurrentRate(conf);
    original_size_id       = XRRConfigCurrentConfiguration(conf, &original_rotation);

    printf("\n\tCURRENT SIZE ID  : %i\n", original_size_id);
    printf("\tCURRENT ROTATION : %i \n", original_rotation);
    printf("\tCURRENT RATE     : %i Hz\n\n", original_rate);

    // GET POSSIBLE SCREEN RESOLUTIONS
    xrrs   = XRRSizes(dpy, 0, &num_sizes);

    // LOOP THROUGH ALL POSSIBLE RESOLUTIONS, GETTING THE SELECTABLE DISPLAY FREQUENCIES
    int i;
    for(i = 0; i < num_sizes; i ++) {
        short   *rates;
        int     num_rates;

        printf("\n\t%2i : %4i x %4i   (%4imm x%4imm ) ", i, xrrs[i].width, xrrs[i].height, xrrs[i].mwidth, xrrs[i].mheight);

        rates = XRRRates(dpy, 0, i, &num_rates);

        for(int j = 0; j < num_rates; j ++) {
                possible_frequencies[i][j] = rates[j];
                printf("%4i ", rates[j]);
        }

        if((xrrs[i].width == WINDOW_WIDTH) && (xrrs[i].height == WINDOW_HEIGHT)){
            // CHANGE RESOLUTION
            printf("*chosen* resolution set to %i x %i PIXELS, %i Hz", xrrs[1].width, xrrs[1].height, possible_frequencies[1][0]);
            XRRSetScreenConfigAndRate(dpy, conf, root, i, RR_Rotate_0, possible_frequencies[1][0], CurrentTime);
        }

    }

    printf("\n");

}

void restore_resolution(){
    // RESTORE ORIGINAL CONFIGURATION
    printf("\tRESTORING %i x %i PIXELS, %i Hz\n\n", xrrs[original_size_id].width, xrrs[original_size_id].height, original_rate);
    XRRSetScreenConfigAndRate(dpy, conf, root, original_size_id, original_rotation, original_rate, CurrentTime);

    // EXIT
    XCloseDisplay(dpy);
}


#include <pthread.h>
/* this function is the second thread */
void *update(void *argv){

    printf("Update thread initializing\r\n");

    //set up the texture pointer and get read to read frames
    jumbotron.setupXorg();

    //update the texture
    while(running){
        jumbotron.grabXorg();
    }

    /* the function must return something - NULL will do */
    return NULL;

}

int main(int argc, char** argv){

    system("echo XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR\r\n");

    const uint8_t FRAME_MS = 16; // 62.5 fps
    float PLAYER_VELOCITY = 0.0f;
    float CAMERA_DISTANCE = 0.0f;
    mouse_dev mouse;
    psvr_dev psvr;
    timer frame_timer;
    int fullscreen = 0;

    //init psvr
    psvr.focus = 4.0f;

    //attempt to connect to psvr
    if ((psvr.retval = psvr_open(&psvr.ctx)) < 0){
        printf("Cannot open PSVR.\r\n");
        psvr.init = 0;
    }
    else if((psvr.retval = psvr_read_sensor_sync(psvr.ctx, (uint8_t *)&psvr.a_frame, sizeof(struct psvr_sensor_frame))) == 0){
        printf("Opened PSVR.\r\n");
        psvr.init = 1;
    }

    //init sdl
    SDL_Init( SDL_INIT_VIDEO );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

    //create SDL window
    static SDL_Window* a_window = SDL_CreateWindow( WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
                             /*SDL_WINDOW_INPUT_GRABBED |*/ SDL_WINDOW_OPENGL );

    //create a GL context with SDL
    SDL_GLContext gl_ctx = SDL_GL_CreateContext(a_window);
    SDL_GL_MakeCurrent(a_window, gl_ctx);

    //set_resolution();
    //SDL_SetWindowFullscreen(a_window, SDL_WINDOW_FULLSCREEN_DESKTOP );

    //culling depth and hinting
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    //set viewport geometry
    glDepthRange(0.0f, 1.0f);
    glPolygonOffset( WINDOW_WIDTH, WINDOW_HEIGHT);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glScissor(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    //ONE LIGHT ONLY ASSHOLE
    glEnable(GL_LIGHTING);

    //figure it out
    //camera.load_texture("textures/reticle.png");
    //camera.genbox((vertex){ 1.0f,  1.0f,  1.0f },(rgba){1,1,1,1}, 0, NO_REPEAT); //set this to 2.0 2.0 0.0001 and it will screw up the collisions so fix it

    debug.dbginit(320, (((320 / 16) * 9)));
    debug.dbgclear(NOCOLOR);
    debug.dbgprint(" Welcome!                             ", 1, 1,  0x88880000, WHITE, 4);
    debug.dbgprint("      Window Size is: %dx%d",   1, 2, WHITE, 0x88880000, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    player.genbox((vertex){ 2.1, 2.1, 0.0001 },(rgba){1,1,1,1}, 0, NO_REPEAT); //set this to 2.0 2.0 0.0001 and it will screw up the collisions so fix it
    player.posrot((vertex){0.2,(MAP_SIZE/2)-14,0.2},(vertex){0,0,0});
    player.b.min  = (vertex){-1.0f, -1.0f, -1.0f};
    player.b.max  = (vertex){ 1.0f,  1.0f,  1.0f};
    player.getspritetex(player_sprite);

    jumbotron.genbox((vertex){ 4, 2.25, 4 },(rgba){1,1,1,1}, 0, NO_REPEAT);
    jumbotron.posrot((vertex){0,8,0},(vertex){0,0,0});

    //jumbotron.load_texture("textures/gridday/grid-solid.png");
    //jumbotron.openframebuffer("/dev/fb0");
    //jumbotron.updateframebuffer();
    //jumbotron.closeframebuffer();
    //jumbotron.setupXorg();
    //jumbotron.grabXorg();

    light.genbox((vertex){ 1, 1, 1 },(rgba){1,1,1,1}, 0, NO_REPEAT);
    light.posrot((vertex){0,6.5,0},(vertex){0,0,0});

    mapbox.genbox((vertex){ 16, 9, 16 },(rgba){1,1,1,1}, 0, NO_REPEAT);
    mapbox.posrot((vertex){0,MAP_SIZE/2,0},(vertex){0,0,0});
    mapbox.load_texture("textures/gridday/crit.png");
    mapbox.f=GL_CW;

    /* create a second thread*/
/*
    pthread_t update_thread;
    if(pthread_create(&update_thread, NULL, &update, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
*/

    //hat input macros
    #define PAD_UP      ((SDL_JoystickGetHat(Joystick1, 0) == SDL_HAT_UP)       || ((u16)(SDL_JoystickGetAxis(Joystick1, 1)) == 32768) || IsKeyDown(SDLK_w))
    #define PAD_DOWN    ((SDL_JoystickGetHat(Joystick1, 0) == SDL_HAT_DOWN)     || ((u16)(SDL_JoystickGetAxis(Joystick1, 1)) == 32767) || IsKeyDown(SDLK_s))
    #define PAD_LEFT    ((SDL_JoystickGetHat(Joystick1, 0) == SDL_HAT_LEFT)     || ((u16)(SDL_JoystickGetAxis(Joystick1, 0)) == 32768) || IsKeyDown(SDLK_a))
    #define PAD_RIGHT   ((SDL_JoystickGetHat(Joystick1, 0) == SDL_HAT_RIGHT)    || ((u16)(SDL_JoystickGetAxis(Joystick1, 0)) == 32767) || IsKeyDown(SDLK_d))

    //initialise the sdl joystick library and create a joystick (?context?)... thingy.
    SDL_Joystick* Joystick1 = NULL;
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    if( SDL_NumJoysticks() < 1) printf("SDL: no joysticks found\r\n");
    else{
        Joystick1 = SDL_JoystickOpen(0);
        if(Joystick1 == NULL) printf("SDL: found controller but unable to open it\r\n");
        else{
            printf("%s Controller Found.\r\n", SDL_JoystickName(Joystick1));
            printf("SDL: controller initialization: %s \r\n", SDL_JoystickName(Joystick1));
            if(strcmp("Sony Interactive Entertainment Controller", SDL_JoystickName(Joystick1)) == 0){
                /* playstation classic controller input macros */
                #define SQUARE                  (SDL_JoystickGetButton(Joystick1, 0) == 1)
                #define CROSS                   (SDL_JoystickGetButton(Joystick1, 1) == 1)
                #define CIRCLE                  (SDL_JoystickGetButton(Joystick1, 2) == 1)
                #define TRIANGLE                (SDL_JoystickGetButton(Joystick1, 3) == 1)
                #define L2                      (SDL_JoystickGetButton(Joystick1, 4) == 1)
                #define R2                      (SDL_JoystickGetButton(Joystick1, 5) == 1)
                #define L1                      (SDL_JoystickGetButton(Joystick1, 6) == 1)
                #define R1                      (SDL_JoystickGetButton(Joystick1, 7) == 1)
                #define SELECT                  (SDL_JoystickGetButton(Joystick1, 8) == 1)
                #define START                   (SDL_JoystickGetButton(Joystick1, 9) == 1)
            }
            else if((strcmp("Sony PLAYSTATION(R)3 Controller", SDL_JoystickName(Joystick1)) == 0) ||
                    (strcmp("Sony Interactive Entertainment Wireless Controller", SDL_JoystickName(Joystick1)) == 0)){
                /* playstation 4 controller input macros */
                #define SQUARE                  (SDL_JoystickGetButton(Joystick1, 3) == 1)
                #define CROSS                   (SDL_JoystickGetButton(Joystick1, 0) == 1)
                #define CIRCLE                  (SDL_JoystickGetButton(Joystick1, 1) == 1)
                #define TRIANGLE                (SDL_JoystickGetButton(Joystick1, 2) == 1)
                #define L2                      (SDL_JoystickGetButton(Joystick1, 4) == 1)
                #define R2                      (SDL_JoystickGetButton(Joystick1, 5) == 1)
                #define L1                      (SDL_JoystickGetButton(Joystick1, 6) == 1)
                #define R1                      (SDL_JoystickGetButton(Joystick1, 7) == 1)
                #define SELECT                  (SDL_JoystickGetButton(Joystick1, 8) == 1)
                #define START                   (SDL_JoystickGetButton(Joystick1, 9) == 1)
            }
        }
    }

    while(running){

        //Events...

        //lets tick!
        frame_timer.st = SDL_GetTicks();

        //check if psvr has been initialized.
        if((psvr.init==1) && (psvr.mode==1)){
            psvr.retval = psvr_read_sensor_sync(psvr.ctx, (uint8_t *)&psvr.a_frame, sizeof(struct psvr_sensor_frame));
            if(psvr.retval == 0){
                int i;
                for(i=0;i<2;i++){
                    //left right
                    short currenty = psvr.a_frame.data[i].gyro.yaw;
                         if(currenty < -100) player.r.y += -psvr.a_frame.data[i].gyro.yaw/PI/360.0f;
                    else if(currenty >  100) player.r.y -=  psvr.a_frame.data[i].gyro.yaw/PI/360.0f;
                    psvr.previousy = currenty;
                    //up down
                    short currentx = psvr.a_frame.data[i].gyro.pitch;
                         if(currentx < -100) camera.r.x += -psvr.a_frame.data[i].gyro.pitch/PI/360.0f;
                    else if(currentx >  100) camera.r.x -=  psvr.a_frame.data[i].gyro.pitch/PI/360.0f;
                    psvr.previousx = currentx;
/*
                    //left right roll
                    short currentz = psvr.a_frame.data[i].gyro.roll;
                         if(currentz < -100) camera.r.z +=  psvr.a_frame.data[i].gyro.roll/PI/360.0f;
                    else if(currentz >  100) camera.r.z -= -psvr.a_frame.data[i].gyro.roll/PI/360.0f;
                    psvr.previousz = currentz;
*/
                }
            }
        }

        //set up mouse
        int mspos_x, mspos_y;
        uint32_t ms_btns = SDL_GetMouseState( &mspos_x, &mspos_y );

        //poll the mouse
        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            if(ev.type == SDL_QUIT){ running = false; }
            if(ev.type == SDL_WINDOWEVENT){
                if ( ev.window.event == SDL_WINDOWEVENT_RESIZED ){
                    int dw, dh;
                    WINDOW_WIDTH  = ev.window.data1;
                    WINDOW_HEIGHT = ev.window.data2;
                    SDL_GL_GetDrawableSize(a_window, &dw, &dh);
                    glPolygonOffset(dw, dh);
                    glViewport(0, 0, dw, dh);
                    glScissor(0, 0, dw, dh);
                    debug.dbgclear(NOCOLOR);
                    debug.dbgprint("      Current Resolution is: %dx%d",   1, 1, WHITE, 0x88880000, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
                }
            }
            if(ev.type == SDL_KEYDOWN){
                if ( ev.key.repeat ) break;
                SDL_Keycode wParam = ev.key.keysym.sym;
                //init shut down sequence implode destroy destroy!
                if(wParam == SDLK_ESCAPE){ running = false; }
                //toggle vr mode
                if(wParam == SDLK_v){
                    if(!psvr.mode) psvr.mode = 1;
                    else psvr.mode = 0;
                }
                //stepping through the focus is better than rolling through it less eye strain..
                if(wParam == SDLK_x){ if(psvr.focus < 4.0f)  psvr.focus+=0.1f; else psvr.focus = -4.0; }
                if(wParam == SDLK_c){ if(psvr.focus > -4.0f) psvr.focus-=0.1f; else psvr.focus =  4.0; }
                //depth axis (camera zoom)
                if(wParam == SDLK_z){ if(CAMERA_DISTANCE < 10) CAMERA_DISTANCE+=2.0f; else CAMERA_DISTANCE=0.0f; }

            }
            if(ev.type == SDL_MOUSEMOTION){
                if(ms_btns){
                    //mouse motion x is player rotation y? weird... but true.
                         short cur_y = ev.motion.x;
                         if(cur_y < mouse.prev_y) player.r.y -= PI;
                    else if(cur_y > mouse.prev_y) player.r.y += PI;
                    mouse.prev_y = cur_y;
                    //mouse motion y is player rotation x? weird... but true.
                         short cur_x = ev.motion.y;
                         if(cur_x < mouse.prev_x) camera.r.x -= PI;
                    else if(cur_x > mouse.prev_x) camera.r.x += PI;
                    mouse.prev_x = cur_x;
                }
            }
        }

        //shut down
        if(IsKeyDown(SDLK_SLEEP)){      running = false; }

        if(IsKeyDown(SDLK_f)){
            if(!fullscreen){ SDL_SetWindowFullscreen(a_window, SDL_WINDOW_FULLSCREEN_DESKTOP ); fullscreen = 1; SDL_Delay(100); }
            else{  SDL_SetWindowFullscreen(a_window, 0 ); fullscreen = 0; SDL_Delay(100); }
        }

        //light height adjustment
        if(IsKeyDown(SDLK_p)){  light.p.y ++; }
        if(IsKeyDown(SDLK_o)){  light.p.y --; }

        //look up down axis
        if(IsKeyDown(SDLK_i)){  camera.r.x -= PI; }
        if(IsKeyDown(SDLK_k)){  camera.r.x += PI; }

        //look left right axis
        if(IsKeyDown(SDLK_j)){  player.r.y -= PI; }
        if(IsKeyDown(SDLK_l)){  player.r.y += PI; }

        //camera update
        if(TRIANGLE == 1){  camera.r.x -= PI; }
        if(CROSS == 1){     camera.r.x += PI; }

        //player update
        if(SQUARE == 1){    player.r.y -= PI; }
        if(CIRCLE == 1){    player.r.y += PI; }

        //alternate shut down
        if(START == 1){     running = false; }
        if(SELECT == 1){    if(CAMERA_DISTANCE < 10) CAMERA_DISTANCE+=2.0f; else CAMERA_DISTANCE=0.0f; }

        //fly i guess lol
        if(IsKeyDown(SDLK_SPACE)){  }

        /** JOYHAT INPUT **/
        //walk controls
        if((PAD_UP == 1) && (PLAYER_VELOCITY < 0.15f)){
            PLAYER_VELOCITY+=0.005f;
            player.heading=2;
        }
        else if((PAD_DOWN == 1) && (PLAYER_VELOCITY < 0.15f)){
            PLAYER_VELOCITY+=0.005f;
            player.heading=0;
        }
        else if((PAD_LEFT == 1) && (PLAYER_VELOCITY < 0.15f)){
            PLAYER_VELOCITY+=0.005f;
            player.heading=1;
        }
        else if((PAD_RIGHT == 1) && (PLAYER_VELOCITY < 0.15f)){
            PLAYER_VELOCITY+=0.005f;
            player.heading=3;
        }
        else if(PLAYER_VELOCITY > 0.0f) PLAYER_VELOCITY -= 0.005f;

        //update player
        if(player.heading==2){ //AWAY FROM CAMERA
            player.p.x += sin(player.r.y / 180 * PI)*PLAYER_VELOCITY;
            player.p.z -= cos(player.r.y / 180 * PI)*PLAYER_VELOCITY;
        }
        else if(player.heading==0){ //TOWARD CAMERA
            player.p.x -= sin(player.r.y / 180 * PI)*PLAYER_VELOCITY;
            player.p.z += cos(player.r.y / 180 * PI)*PLAYER_VELOCITY;
        }
        else if(player.heading==1){ //LEFT
            player.p.x += sin(80.1f+player.r.y / 180 * PI)*PLAYER_VELOCITY;
            player.p.z -= cos(80.1f+player.r.y / 180 * PI)*PLAYER_VELOCITY;
        }
        else if(player.heading==3){ //RIGHT
            player.p.x -= sin(80.1f+player.r.y / 180 * PI)*PLAYER_VELOCITY;
            player.p.z += cos(80.1f+player.r.y / 180 * PI)*PLAYER_VELOCITY;
        }

        //player physics
        player.grav();
        player.col(mapbox, GL_CW);
        //player.col(jumbotron, GL_CCW);

        //jumbotron physics
        //jumbotron.grav();
        //jumbotron.col(mapbox, GL_CW);
        //jumbotron.grabXorg();

        //set up scene
        glClearColor(0.01f, 0.01f, 0.01f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        //with psvr
        if(psvr.mode == 1){
            //left eye
            glViewport(0, 0, WINDOW_WIDTH/2, WINDOW_HEIGHT);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            {
                Perspective(75.0f, (float)(WINDOW_WIDTH/2)/WINDOW_HEIGHT, 0.1f, WINDOW_WIDTH);
                glRotatef(camera.r.x, 1, 0, 0);
                glRotatef(player.r.y-psvr.focus, 0, 1, 0);
                glRotatef(player.r.z, 0, 0, 1);
                glTranslatef(-player.p.x+sin(player.r.y / 180 * PI)*CAMERA_DISTANCE, -player.p.y-(CAMERA_DISTANCE/4), -player.p.z-cos(player.r.y / 180 * PI)*CAMERA_DISTANCE);
            }
            DrawScene();
            //right eye
            glViewport(WINDOW_WIDTH/2, 0, WINDOW_WIDTH/2, WINDOW_HEIGHT);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            {
                Perspective(75.0f, (float)(WINDOW_WIDTH/2)/WINDOW_HEIGHT, 0.1f, 1000.0f);
                glRotatef(camera.r.x, 1, 0, 0);
                glRotatef(player.r.y+psvr.focus, 0, 1, 0);
                glRotatef(player.r.z, 0, 0, 1);
                glTranslatef(-player.p.x+sin(player.r.y / 180 * PI)*CAMERA_DISTANCE, -player.p.y-(CAMERA_DISTANCE/4), -player.p.z-cos(player.r.y / 180 * PI)*CAMERA_DISTANCE);
            }
            DrawScene();
        }
        else{
            //without psvr
            glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            {
                Perspective(75.0f, (float)WINDOW_WIDTH/WINDOW_HEIGHT, 0.1f, 1000.0f);
                glRotatef(camera.r.x, 1, 0, 0);
                glRotatef(player.r.y, 0, 1, 0);
                glRotatef(player.r.z, 0, 0, 1);
                glTranslatef(-player.p.x+sin(player.r.y / 180 * PI)*CAMERA_DISTANCE, -player.p.y-CAMERA_DISTANCE/3, -player.p.z-cos(player.r.y / 180 * PI)*CAMERA_DISTANCE);
            }
            DrawScene();
        }

        //flipity flip
        SDL_GL_SwapWindow(a_window);

        //frame rate stuff
        frame_timer.ms = SDL_GetTicks() - frame_timer.st;
        //frame_timer.delay = FRAME_MS - (frame_timer.ms); //makes it fly
        frame_timer.delay = frame_timer.delay > FRAME_MS ? 0 : frame_timer.delay;

        printf("%f - fps\r\n", 1.0 / (frame_timer.ms/1000.0f));

        SDL_Delay(frame_timer.delay);

    }

    //shut down SDL
    SDL_DestroyWindow(a_window);
    SDL_JoystickClose(Joystick1);

    /* wait for the second thread to finish */
/*
    if(pthread_join(update_thread, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }
*/

    //restore_resolution();

    return 0;

}

