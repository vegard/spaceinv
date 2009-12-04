#ifndef SPACEINV_ALIEN_HH
#define SPACEINV_ALIEN_HH

class spaceinv_alien: public object {
public:
	spaceinv_alien(unsigned int nr_frames, texture** textures);
	virtual ~spaceinv_alien();

public:
	cpVect position();
	cpVect velocity();

	void draw();
	void add_to_space(cpSpace *s);
	void remove_from_space(cpSpace *s);

public:
	unsigned int _nr_frames;
	texture **_textures;

	unsigned int _switch_frame;
	unsigned int _frame;

	cpBody *_body;
	cpShape *_shape;
};

spaceinv_alien::spaceinv_alien(unsigned int nr_frames, texture **textures):
	_nr_frames(nr_frames),
	_textures(textures),
	_switch_frame(0),
	_frame(0)
{
	_body = cpBodyNew(10, INFINITY);

	_shape = cpCircleShapeNew(_body, cpvlength(cpv(4, 4)), cpvzero);
	_shape->data = this;
	_shape->collision_type = COLLISION_TYPE_SHIP;
	_shape->e = 0;
	_shape->u = 0.5;
}

spaceinv_alien::~spaceinv_alien()
{
	cpBodyDestroy(_body);
	cpShapeDestroy(_shape);
}

cpVect
spaceinv_alien::position()
{
	return _body->p;
}

cpVect
spaceinv_alien::velocity()
{
	return _body->v;
}

void
spaceinv_alien::draw()
{
	draw_sprite(_textures[_frame], _body->p.x, _body->p.y, 0);

	if (++_switch_frame == CONFIG_FPS / 2) {
		_switch_frame = 0;

		if (++_frame == _nr_frames)
			_frame = 0;
	}
}

void
spaceinv_alien::add_to_space(cpSpace *s)
{
	cpSpaceAddBody(s, _body);
	cpSpaceAddShape(s, _shape);
}

void
spaceinv_alien::remove_from_space(cpSpace *s)
{
	cpSpaceRemoveShape(s, _shape);
	cpSpaceRemoveBody(s, _body);
}

#endif
