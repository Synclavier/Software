/*  Metrowerks Standard Library  Version 2.1.2  1997 May  *//* This file is only required when using the SIOUX source code to   compile, e.g. building a new library. It sets the preprocessor   options to include WASTE support.      If your SIOUX lib is compiled properly (i.e. you're using an   MSL-compatible lib in an MSL project), you do not need to include   this file--just SIOUX.h if you want to change SIOUX settings.      --pcg*/#include <SIOUXPrefix.h>#define SIOUX_USE_WASTE		1#define WASTE_IC_SUPPORT	1#define WASTE_DEBUG			0#define	WASTE_OBJECTS		0