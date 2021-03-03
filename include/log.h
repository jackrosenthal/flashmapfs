#ifndef _FMAPFS_LOG_H
#define _FMAPFS_LOG_H

#include <stdio.h>

#include "is_enabled.h"

#define LOG_ERR(args...) _LOG1(ERROR, __FILE__, __LINE__, __func__, args)
#define LOG_WARN(args...) _LOG1(WARNING, __FILE__, __LINE__, __func__, args)
#define LOG_INF(args...) _LOG1(INFO, __FILE__, __LINE__, __func__, args)
#define LOG_DBG(args...) _LOG1(DEBUG, __FILE__, __LINE__, __func__, args)

#define _LOG1(level, file, line, function, fmt, args...)                   \
	do {                                                               \
		if (IS_ENABLED(CONFIG_LOG_LEVEL_##level##_ENABLED)) {      \
			fprintf(stderr, "[%s %s:%d/%s] " fmt "\n", #level, \
				file, line, function, ##args);             \
		}                                                          \
	} while (0)

#endif /* _FMAPFS_LOG_H */
