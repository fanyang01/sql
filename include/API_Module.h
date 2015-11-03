#ifndef _API_MODULE_H
#define _API_MODULE_H

#include "API_Module.h"
#include "Interpreter.h"
#include "db.h"

#define STRING_MAX 255

void API_Module(string sql);
extern DB *db;

#endif
