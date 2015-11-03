#include "db.h"
#include "API_Module.h"
#include "Interpreter.h"
#include <stdlib.h>

DB *db;

int main() {
	if((db = opendb("/tmp/tmp.db", O_RDWR | O_CREAT, 0644)) == NULL) {
		perror("FATAL ERROR");
		exit(1);
	}
	while(true) {
		API_Module(Interpreter(read_input()));
	}
}
