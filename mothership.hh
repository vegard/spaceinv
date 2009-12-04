#ifndef MOTHERSHIP_HH
#define MOTHERSHIP_HH

class mothership: public object {
public:
	mothership();
	virtual ~mothership();

public:
	cpVect position();
	cpVect velocity();

	void draw();
	void add_to_space(cpSpace *s);
	void remove_from_space(cpSpace *s);

public:
	cpBody *_body;
	cpShape *_shape[2];
};

mothership::mothership()
{
	_body = cpBodyNew(10, INFINITY);

	_shape[0] = cpCircleShapeNew(_body, 4, cpv(-4, 0));
	_shape[0]->data = this;
	_shape[0]->collision_type = COLLISION_TYPE_SHIP;
	_shape[0]->e = 0;
	_shape[0]->u = 0.5;
	_shape[1] = cpCircleShapeNew(_body, 4, cpv(4, 0));
	_shape[1]->data = this;
	_shape[1]->collision_type = COLLISION_TYPE_SHIP;
	_shape[1]->e = 0;
	_shape[1]->u = 0.5;
}

mothership::~mothership()
{
	cpBodyDestroy(_body);
	cpShapeDestroy(_shape[0]);
	cpShapeDestroy(_shape[1]);
}

cpVect
mothership::position()
{
	return _body->p;
}

cpVect
mothership::velocity()
{
	return _body->v;
}

void
mothership::draw()
{
	draw_sprite(mothership_texture, _body->p.x, _body->p.y, 0);
//		360 * cpvtoangle(_body->rot) / M_PI / 2);
}

void
mothership::add_to_space(cpSpace *s)
{
	cpSpaceAddBody(s, _body);
	cpSpaceAddShape(s, _shape[0]);
	cpSpaceAddShape(s, _shape[1]);
}

void
mothership::remove_from_space(cpSpace *s)
{
	cpSpaceRemoveShape(s, _shape[0]);
	cpSpaceRemoveShape(s, _shape[1]);
	cpSpaceRemoveBody(s, _body);
}

#endif
