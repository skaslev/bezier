#ifndef UTIL_H
#define UTIL_H

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define SWAP(type, a, b)			\
	do {					\
		type __tmp = (a);		\
		(a) = (b);			\
		(b) = __tmp;			\
	} while (0)

#endif
