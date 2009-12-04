extern "C" {
#include <stdint.h>
}

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>

extern "C" {
#include <SDL.h>
#include <SDL_opengl.h>
#include <chipmunk.h>
#include <png.h>
}


#define CONFIG_DEBUG_HALFSIZE 0
#define CONFIG_DEBUG_DRAW_SPRITE_OUTLINE 0

#define CONFIG_FPS 60
#define CONFIG_PHYSICS_STEP_SIZE 1e-1

#define CONFIG_CAPTURE 0
#define CONFIG_CAPTURE_FRAMES 0

/* Size of the window */
#define CONFIG_WINDOW_WIDTH 1280
#define CONFIG_WINDOW_HEIGHT 768

/* Size of the game screen */
#define CONFIG_SCREEN_WIDTH 128
#define CONFIG_SCREEN_HEIGHT 128

/* Choice: */
#define CONFIG_CONTROL CONFIG_CONTROL_POLAR
#define CONFIG_CONTROL_CARTESIAN 0
#define CONFIG_CONTROL_POLAR 1

#define CONFIG_CONTROL_CARTESIAN_FORCE 30
#define CONFIG_CONTROL_POLAR_TORQUE (M_PI / 4.)
#define CONFIG_CONTROL_POLAR_FORCE 45


#include "capture.hh"
#include "texture.hh"


enum collision_type {
	COLLISION_TYPE_NONE,
	COLLISION_TYPE_SHIP,
	COLLISION_TYPE_BULLET,
};

enum collision_group {
	COLLISION_GROUP_NONE,
	COLLISION_GROUP_BULLET,
};

static cpSpace *space;

static texture* starfield_texture[2];
static texture* mothership_texture;
static texture* galaga1_texture[6];
static texture* spaceinv_alien1_texture[2];
static texture* spaceinv_alien2_texture[2];
static texture* spaceinv_alien3_texture[2];
static texture* galaga_bullet_texture;
static texture* spaceinv_bullet1_texture;
static texture* explosion_texture[5];

class object {
public:
	object();
	virtual ~object();

public:
	virtual cpVect position() = 0;
	virtual cpVect velocity() = 0;

	virtual void draw();
	virtual void add_to_space(cpSpace *s);
	virtual void remove_from_space(cpSpace *s);
};

object::object()
{
}

object::~object()
{
}

void
object::draw()
{
}

void
object::add_to_space(cpSpace *s)
{
}

void
object::remove_from_space(cpSpace *s)
{
}

static void
draw_sprite(texture *tex, double x, double y, double rot)
{
	glPushMatrix();

	/* Set up the new (local) coordinate system so that the coordinate
	 * (-1, -1) refers to the upper-left corner of the texture and
	 * (1, 1) refers to the lower-right corner. */
	//glTranslatef(trunc(x), trunc(y), 0);
	glTranslatef(x, y, 0);
	glRotatef(rot, 0, 0, 1);
	glScalef(tex->_width / 2., tex->_height / 2., 1);

	tex->bind();
	glColor4f(1, 1, 1, 1);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-1, -1);

	glTexCoord2f(1, 0);
	glVertex2f(1, -1);

	glTexCoord2f(1, 1);
	glVertex2f(1, 1);

	glTexCoord2f(0, 1);
	glVertex2f(-1, 1);
	glEnd();

#if CONFIG_DEBUG_DRAW_SPRITE_OUTLINE
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_LINES);
	glVertex2f(-1, -1);
	glVertex2f(1, -1);

	glVertex2f(1, -1);
	glVertex2f(1, 1);

	glVertex2f(1, 1);
	glVertex2f(-1, 1);

	glVertex2f(-1, 1);
	glVertex2f(-1, -1);
	glEnd();
#endif

	glPopMatrix();
}

typedef std::set<object*> object_set;

static object_set objects;
static object_set removed_objects;
static object_set exploded_objects;

/* These are not proper header files: */
#include "bullet.hh"
#include "explosion.hh"
#include "mothership.hh"
#include "ship.hh"
#include "spaceinv_alien.hh"

static void
add_object(object* o)
{
	objects.insert(o);
	o->add_to_space(space);
}

static void
remove_object(object* o)
{
	objects.erase(o);
	o->remove_from_space(space);
}

enum bullet_type {
	BULLET_TYPE_GALAGA,
	BULLET_TYPE_SPACEINV,

	NR_BULLET_TYPES,
};

static texture *const *bullet_textures[] = {
	&galaga_bullet_texture,
	&spaceinv_bullet1_texture,
};

static ship *player;
static bullet_type player_bullet_type = BULLET_TYPE_SPACEINV;

static cpVect camera_p = cpvzero;
static cpVect camera_v = cpvzero;

static void camera_update(void)
{
	cpVect p = player->_body->p;
	cpVect v = player->_body->v;

	/* Friction */
	camera_v = cpvmult(camera_v, 0.9);

	/* Spring force */
	camera_v = cpvadd(camera_v, cpvmult(cpvsub(p, camera_p), 1e-3));

	/* Damping */
	camera_v = cpvadd(camera_v, cpvmult(cpvsub(v, camera_v), 1e-2));

	camera_p = cpvadd(camera_p, camera_v);
}

static int
ship_bullet_collision(cpShape *shape_a, cpShape *shape_b,
	cpContact *contacts, int nr_contacts, cpFloat normal_coef, void *data)
{
	object *s = (object *) shape_a->data;
	object *b = (object *) shape_b->data;

	exploded_objects.insert(s);
	removed_objects.insert(b);
	return 1;
}

static int
ship_ship_collision(cpShape *shape_a, cpShape *shape_b,
	cpContact *contacts, int nr_contacts, cpFloat normal_coef, void *data)
{
	object *a = (object *) shape_a->data;
	object *b = (object *) shape_b->data;

	exploded_objects.insert(a);
	exploded_objects.insert(b);
	return 1;
}

static void
init()
{
	cpInitChipmunk();

	srand(time(NULL));

	space = cpSpaceNew();
	space->elasticIterations = 10;
	space->gravity = cpvzero;

	cpSpaceAddCollisionPairFunc(space,
		COLLISION_TYPE_SHIP, COLLISION_TYPE_BULLET,
		&ship_bullet_collision, NULL);
	cpSpaceAddCollisionPairFunc(space,
		COLLISION_TYPE_SHIP, COLLISION_TYPE_SHIP,
		&ship_ship_collision, NULL);

	starfield_texture[0] = texture::get_png("starfield-1.png");
	starfield_texture[1] = texture::get_png("starfield-2.png");
	mothership_texture = texture::get_png("spaceinv-mothership.png");
	galaga1_texture[0] = texture::get_png("galaga-1-1.png");
	galaga1_texture[1] = texture::get_png("galaga-1-2.png");
	galaga1_texture[2] = texture::get_png("galaga-1-3.png");
	galaga1_texture[3] = texture::get_png("galaga-1-4.png");
	galaga1_texture[4] = texture::get_png("galaga-1-5.png");
	galaga1_texture[5] = texture::get_png("galaga-1-6.png");
	spaceinv_alien1_texture[0] = texture::get_png("spaceinv-alien1-1.png");
	spaceinv_alien1_texture[1] = texture::get_png("spaceinv-alien1-2.png");
	spaceinv_alien2_texture[0] = texture::get_png("spaceinv-alien2-1.png");
	spaceinv_alien2_texture[1] = texture::get_png("spaceinv-alien2-2.png");
	spaceinv_alien3_texture[0] = texture::get_png("spaceinv-alien3-1.png");
	spaceinv_alien3_texture[1] = texture::get_png("spaceinv-alien3-2.png");
	galaga_bullet_texture = texture::get_png("galaga-bullet.png");
	spaceinv_bullet1_texture = texture::get_png("spaceinv-bullet-1.png");
	explosion_texture[0] = texture::get_png("galaga-explosion-2-1.png");
	explosion_texture[1] = texture::get_png("galaga-explosion-2-2.png");
	explosion_texture[2] = texture::get_png("galaga-explosion-2-3.png");
	explosion_texture[3] = texture::get_png("galaga-explosion-2-4.png");
	explosion_texture[4] = texture::get_png("galaga-explosion-2-5.png");

	for (unsigned int i = 0; i < 20; ++i) {
		mothership *m = new mothership();
		m->_body->p = cpv(rand() % 1024 - 512, rand() % 1024 - 512);
		add_object(m);
	}

	for (unsigned int i = 0; i < 10; ++i) {
		spaceinv_alien *a = new spaceinv_alien(2,
			spaceinv_alien1_texture);
		a->_body->p = cpv(rand() % 1024 - 512, rand() % 1024 - 512);
		add_object(a);
	}

	for (unsigned int i = 0; i < 10; ++i) {
		spaceinv_alien *a = new spaceinv_alien(2,
			spaceinv_alien2_texture);
		a->_body->p = cpv(rand() % 1024 - 512, rand() % 1024 - 512);
		add_object(a);
	}

	for (unsigned int i = 0; i < 10; ++i) {
		spaceinv_alien *a = new spaceinv_alien(2,
			spaceinv_alien3_texture);
		a->_body->p = cpv(rand() % 1024 - 512, rand() % 1024 - 512);
		add_object(a);
	}

	player = new ship();
	add_object(player);
}

static void
deinit()
{
}

static void
resize(int width, int height)
{
	if(width == 0 || height == 0)
		return;

	double aspect = 1.0 * height / width;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluOrtho2D(-CONFIG_SCREEN_WIDTH, CONFIG_SCREEN_WIDTH,
		CONFIG_SCREEN_HEIGHT * aspect, -CONFIG_SCREEN_HEIGHT * aspect);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void draw_starfield(texture *tex, double scale)
{
	/* XXX: Calculate starfield based on screen and texture size. */
	double o_x = camera_p.x / scale;
	double o_y = camera_p.y / scale;

	int d_x = (int) trunc(o_x / 64.) - 1;
	int d_y = (int) trunc(o_y / 64.) - 1;

	for (int y = 0; y < 6; ++y) {
		for (int x = 0; x < 6; ++x) {
			draw_sprite(tex,
				-o_x + 64 * (d_x + x) - 32 - 64,
				-o_y + 64 * (d_y + y) - 32 - 64, 0);
		}
	}
}

static void
display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();
#if 0
	glTranslatef(-(camera_p.x - trunc(camera_p.x)),
		-(camera_p.y - trunc(camera_p.y)), 0);
#endif

#if CONFIG_DEBUG_HALFSIZE
	glPushMatrix();
	glScalef(0.5, 0.5, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1, 1, 1, 1);

	glBegin(GL_LINES);
	glVertex2f(-CONFIG_SCREEN_WIDTH, -CONFIG_SCREEN_HEIGHT);
	glVertex2f(-CONFIG_SCREEN_WIDTH,  CONFIG_SCREEN_HEIGHT);

	glVertex2f(-CONFIG_SCREEN_WIDTH,  CONFIG_SCREEN_HEIGHT);
	glVertex2f( CONFIG_SCREEN_WIDTH,  CONFIG_SCREEN_HEIGHT);

	glVertex2f( CONFIG_SCREEN_WIDTH,  CONFIG_SCREEN_HEIGHT);
	glVertex2f( CONFIG_SCREEN_WIDTH, -CONFIG_SCREEN_HEIGHT);

	glVertex2f( CONFIG_SCREEN_WIDTH, -CONFIG_SCREEN_HEIGHT);
	glVertex2f(-CONFIG_SCREEN_WIDTH, -CONFIG_SCREEN_HEIGHT);
	glEnd();
#endif

	draw_starfield(starfield_texture[0], 2);
	draw_starfield(starfield_texture[1], 3);

	glTranslatef(-camera_p.x, -camera_p.y, 0);

	for (object_set::iterator i = objects.begin(), end = objects.end();
		i != end; ++i)
	{
		(*i)->draw();
	}

#if CONFIG_DEBUG_HALFSIZE
	glPopMatrix();
#endif
	glPopMatrix();

	SDL_GL_SwapBuffers();

	capture();

	cpSpaceStep(space, CONFIG_PHYSICS_STEP_SIZE);
	camera_update();

	for (object_set::iterator i = removed_objects.begin(),
		end = removed_objects.end();
		i != end; ++i)
	{
		remove_object(*i);
	}

	removed_objects.clear();

	for (object_set::iterator i = exploded_objects.begin(),
		end = exploded_objects.end();
		i != end; ++i)
	{
		object *o = *i;
		add_object(new explosion(o->position(), o->velocity()));
		remove_object(o);
		delete o;

		if (o == player) {
			player = new ship();
			add_object(player);
		}
	}

	exploded_objects.clear();
}

static Uint32
displayTimer(Uint32 interval, void* param)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	SDL_PushEvent(&event);
	return interval;
}

static void
keyboard(SDL_KeyboardEvent* key)
{
	switch(key->keysym.sym) {
#if CONFIG_CONTROL == CONFIG_CONTROL_CARTESIAN
	case SDLK_DOWN:
		cpBodyApplyForce(player->_body,
			cpv(0, CONFIG_CONTROL_CARTESIAN_FORCE), cpvzero);
		break;
	case SDLK_UP:
		cpBodyApplyForce(player->_body,
			cpv(0, -CONFIG_CONTROL_CARTESIAN_FORCE), cpvzero);
		break;
	case SDLK_LEFT:
		cpBodyApplyForce(player->_body,
			cpv(-CONFIG_CONTROL_CARTESIAN_FORCE, 0), cpvzero);
		break;
	case SDLK_RIGHT:
		cpBodyApplyForce(player->_body,
			cpv(CONFIG_CONTROL_CARTESIAN_FORCE, 0), cpvzero);
		break;
#endif
#if CONFIG_CONTROL == CONFIG_CONTROL_POLAR
	case SDLK_DOWN:
		break;
	case SDLK_UP:
		cpBodyApplyImpulse(player->_body,
			cpvmult(player->_body->rot,
				CONFIG_CONTROL_POLAR_FORCE), cpvzero);
		break;
	case SDLK_LEFT:
		player->_body->w -= CONFIG_CONTROL_POLAR_TORQUE;
		break;
	case SDLK_RIGHT:
		player->_body->w += CONFIG_CONTROL_POLAR_TORQUE;
		break;
#endif
	case SDLK_TAB: {
		player_bullet_type = (bullet_type)
			((int) player_bullet_type + 1);
		if (player_bullet_type == NR_BULLET_TYPES)
			player_bullet_type = (bullet_type) 0;
		break;
	}
	case SDLK_SPACE: {
		bullet *b = new bullet(*bullet_textures[player_bullet_type],
			2 * CONFIG_FPS);
		b->_body->p = cpvadd(player->_body->p,
			cpvmult(player->_body->rot, 10));
		b->_body->v = cpvadd(player->_body->v,
			cpvmult(player->_body->rot, 20));
		cpBodySetAngle(b->_body,
			cpvtoangle(player->_body->rot));
		add_object(b);
		break;
	}
	case SDLK_F1:
		break;
	case SDLK_ESCAPE: {
			SDL_Event event;
			event.type = SDL_QUIT;
			SDL_PushEvent(&event);
		}
		break;
	default:
		printf("key %d\n", key->keysym.sym);
	}
}

static void
keyboardUp(SDL_KeyboardEvent* key)
{
	switch(key->keysym.sym) {
#if CONFIG_CONTROL == CONFIG_CONTROL_CARTESIAN
	case SDLK_DOWN:
		cpBodyApplyForce(player->_body,
			cpv(0, -CONFIG_CONTROL_CARTESIAN_FORCE), cpvzero);
		break;
	case SDLK_UP:
		cpBodyApplyForce(player->_body,
			cpv(0, CONFIG_CONTROL_CARTESIAN_FORCE), cpvzero);
		break;
	case SDLK_LEFT:
		cpBodyApplyForce(player->_body,
			cpv(CONFIG_CONTROL_CARTESIAN_FORCE, 0), cpvzero);
		break;
	case SDLK_RIGHT:
		cpBodyApplyForce(player->_body,
			cpv(-CONFIG_CONTROL_CARTESIAN_FORCE, 0), cpvzero);
		break;
#endif
#if CONFIG_CONTROL == CONFIG_CONTROL_POLAR
	case SDLK_DOWN:
		break;
	case SDLK_UP:
		break;
	case SDLK_LEFT:
		player->_body->w += CONFIG_CONTROL_POLAR_TORQUE;
		break;
	case SDLK_RIGHT:
		player->_body->w -= CONFIG_CONTROL_POLAR_TORQUE;
		break;
#endif
	default:
		break;
	}
}

int
main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
		fprintf(stderr, "error: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	SDL_Surface* surface = SDL_SetVideoMode(CONFIG_WINDOW_WIDTH,
		CONFIG_WINDOW_HEIGHT, 0, SDL_OPENGL);
	if (!surface) {
		fprintf(stderr, "error: %s\n", SDL_GetError());
		exit(1);
	}

	init();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);

	glClearColor(0, 0, 0, 1);

	resize(CONFIG_WINDOW_WIDTH, CONFIG_WINDOW_HEIGHT);

#if CONFIG_CAPTURE == 0
	SDL_TimerID display_timer = SDL_AddTimer(1000 / CONFIG_FPS,
		&displayTimer, NULL);
	if (!display_timer) {
		fprintf(stderr, "error: %s\n", SDL_GetError());
		exit(1);
	}
#endif

	bool running = true;
	while (running) {
		SDL_Event event;

#if CONFIG_CAPTURE
		display();
		while (SDL_PollEvent(&event))
#else
		SDL_WaitEvent(&event);
#endif
		switch (event.type) {
		case SDL_USEREVENT:
			display();
			break;
		case SDL_KEYDOWN:
			keyboard(&event.key);
			break;
		case SDL_KEYUP:
			keyboardUp(&event.key);
			break;
		case SDL_MOUSEBUTTONDOWN:
			break;
		case SDL_QUIT:
			running = false;
			break;
		case SDL_VIDEORESIZE:
			resize(event.resize.w, event.resize.h);
			break;
		}
	}

	deinit();

	SDL_Quit();
	return 0;
}
