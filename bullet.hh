#ifndef BULLET_HH
#define BULLET_HH

class bullet: public object {
public:
	bullet(texture *texture, unsigned int ttl);
	virtual ~bullet();

public:
	cpVect position();
	cpVect velocity();

	void draw();
	void add_to_space(cpSpace *s);
	void remove_from_space(cpSpace *s);

public:
	texture *_texture;
	unsigned int _ttl;

	cpBody *_body;
	cpShape *_shape;
};

bullet::bullet(texture *texture, unsigned int ttl):
	_texture(texture),
	_ttl(ttl)
{
	_body = cpBodyNew(1, 10);
	_shape = cpCircleShapeNew(_body, texture->_width / 2., cpvzero);
	_shape->data = this;
	_shape->collision_type = COLLISION_TYPE_BULLET;
	_shape->e = 0;
	_shape->u = 0.5;
}

bullet::~bullet()
{
	cpBodyDestroy(_body);
	cpShapeDestroy(_shape);
}

cpVect
bullet::position()
{
	return _body->p;
}

cpVect
bullet::velocity()
{
	return _body->v;
}

void
bullet::draw()
{
	draw_sprite(_texture, _body->p.x, _body->p.y,
		360 * cpvtoangle(_body->rot) / M_PI / 2 + 90);

	if (--_ttl == 0)
		removed_objects.insert(this);
}

void
bullet::add_to_space(cpSpace *s)
{
	cpSpaceAddBody(s, _body);
	cpSpaceAddShape(s, _shape);
}

void
bullet::remove_from_space(cpSpace *s)
{
	cpSpaceRemoveShape(s, _shape);
	cpSpaceRemoveBody(s, _body);
}

#endif
