#ifndef CONSOLE_HH
#define CONSOLE_HH

#include <list>

class message {
public:
	message(char *text, unsigned int ttl);
	~message();

public:
	char *_text;
	unsigned int _ttl;
};

message::message(char *text, unsigned int ttl):
	_text(text),
	_ttl(ttl)
{
}

message::~message()
{
	delete[] _text;
}

typedef std::list<message *> message_list;

static message_list messages;

static inline void
console_printf(unsigned int ttl, const char *fmt, ...)
{
	va_list ap;
	char *text;

	va_start(ap, fmt);
	vasprintf(&text, fmt, ap);
	va_end(ap);

	messages.push_back(new message(text, ttl));
}

static void
draw_string(unsigned int x, unsigned int y, const char *str);

static inline void
console_draw(void)
{
	unsigned int m_y = 1;
	for (message_list::iterator i = messages.begin(),
		end = messages.end(); i != end; ++i)
	{
		message* m = *i;

		draw_string(1, m_y++, m->_text);
	}

	if (messages.size()) {
		message* m = messages.front();

		if (--m->_ttl == 0) {
			messages.pop_front();
			delete m;
		}
	}
}

#endif
