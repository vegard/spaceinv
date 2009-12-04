#ifndef EXPLOSION_HH
#define EXPLOSION_HH

class explosion: public object {
public:
	explosion(cpVect p, cpVect v);
	virtual ~explosion();

public:
	cpVect position();
	cpVect velocity();

	void draw();
	void add_to_space(cpSpace *s);
	void remove_from_space(cpSpace *s);

public:
	unsigned int _switch_frame;
	unsigned int _frame;

	cpBody *_body;
	cpShape *_shape;
};

explosion::explosion(cpVect p, cpVect v):
	_switch_frame(0),
	_frame(0)
{
	_body = cpBodyNew(1, 10);
	_body->p = p;
	_body->v = v;
	_shape = cpCircleShapeNew(_body, 16, cpvzero);
	_shape->data = this;
	_shape->collision_type = COLLISION_TYPE_BULLET;
	_shape->group = COLLISION_GROUP_BULLET;
	_shape->e = 0;
	_shape->u = 0.5;
}

explosion::~explosion()
{
	cpBodyDestroy(_body);
	cpShapeDestroy(_shape);
}

cpVect
explosion::position()
{
	return _body->p;
}

cpVect
explosion::velocity()
{
	return _body->v;
}

void
explosion::draw()
{
	draw_sprite(explosion_texture[_frame], _body->p.x, _body->p.y, 0);

	if (++_switch_frame == CONFIG_FPS / 10) {
		_switch_frame = 0;

		if (++_frame == 6)
			removed_objects.insert(this);
	}
}

void
explosion::add_to_space(cpSpace *s)
{
	cpSpaceAddBody(s, _body);
	cpSpaceAddShape(s, _shape);
}

void
explosion::remove_from_space(cpSpace *s)
{
	cpSpaceRemoveShape(s, _shape);
	cpSpaceRemoveBody(s, _body);
}

#endif
