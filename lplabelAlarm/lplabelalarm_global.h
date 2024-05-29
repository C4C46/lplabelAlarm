#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(LPLABELALARM_LIB)
#  define LPLABELALARM_EXPORT Q_DECL_EXPORT
# else
#  define LPLABELALARM_EXPORT Q_DECL_IMPORT
# endif
#else
# define LPLABELALARM_EXPORT
#endif
