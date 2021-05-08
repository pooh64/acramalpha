#pragma once

#include <stdlib.h>

#define logger(fmt, ...)                                                       \
	do {                                                                   \
		fprintf(stderr, fmt, ##__VA_ARGS__);                           \
		fprintf(stderr, "\n");                                         \
	} while (0)

#define logger_fatal(fmt, ...)                                                 \
	do {                                                                   \
		logger(fmt, ##__VA_ARGS__);                                    \
		exit(1);                                                       \
	} while (0)
