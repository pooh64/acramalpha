#pragma once

#define logger(fmt, ...)                                                          \
	do {                                                                   \
		fprintf(stderr, fmt, ##__VA_ARGS__);                           \
		fprintf(stderr, "\n");                                         \
	} while (0)
