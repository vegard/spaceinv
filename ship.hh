#ifndef SHIP_HH
#define SHIP_HH

class ship: public object {
public:
	ship();
	virtual ~ship();

public:
	cpVect position();
	cpVect velocity();

	void draw();
	void add_to_space(cpSpace *s);
	void remove_from_space(cpSpace *s);

public:
	cpBody *_body;
	cpShape *_shape;
};

ship::ship()
{
	_body = cpBodyNew(10, INFINITY);
	_shape = cpCircleShapeNew(_body, 6, cpvzero);
	_shape->data = this;
	_shape->collision_type = COLLISION_TYPE_SHIP;
	_shape->e = 0;
	_shape->u = 0.5;
}

ship::~ship()
{
	cpBodyDestroy(_body);
	cpShapeDestroy(_shape);
}

cpVect
ship::position()
{
	return _body->p;
}

cpVect
ship::velocity()
{
	return _body->v;
}

void
ship::draw()
{
	cpFloat a = cpvtoangle(_body->rot) + M_PI;
	unsigned int x = trunc(4 * 6 * a / M_PI / 2);

	draw_sprite(galaga1_texture[x % 6], _body->p.x, _body->p.y,
		90. * (x / 6));
}

void
ship::add_to_space(cpSpace *s)
{
	cpSpaceAddBody(s, _body);
	cpSpaceAddShape(s, _shape);
}

void
ship::remove_from_space(cpSpace *s)
{
	cpSpaceRemoveShape(s, _shape);
	cpSpaceRemoveBody(s, _body);
}

#endif
