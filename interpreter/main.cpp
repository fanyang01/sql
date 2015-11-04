#include "db.h"
#include "API_Module.h"
#include "Interpreter.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

const char *TMP_FILE = "/tmp/tmp.db";
const char *prompt = "\033[1;36moursql=>\033[0m ";
DB *db;
string(*readsql) ();
static string _readline();
// readline
int _rl_init(void);
int _rl_bind_cr(int count, int key);

int main(int argc, char *argv[])
{
	using namespace std;

	switch (argc) {
	case 1:
		if ((db = opendb(TMP_FILE,
				 O_RDWR | O_TRUNC | O_CREAT, 0644)) == NULL) {
			perror("FATAL ERROR");
			exit(1);
		}
		unlink(TMP_FILE);
		break;
	case 2:
		if ((db = opendb(argv[1], O_RDWR | O_CREAT, 0644)) == NULL) {
			perror("FATAL ERROR");
			exit(1);
		}
		break;
	default:
		cerr << "Usage: " << argv[0] << " FILENAME" << endl;
		break;
	}

	if (isatty(STDIN_FILENO)) {
		cout << "Welcome to OURSQL!" << endl;
		readsql = _readline;
		rl_startup_hook = _rl_init;
	} else
		readsql = read_input;

	while (true) {
		API_Module(Interpreter(readsql()));
	}
}

string _readline()
{
	char *line;
	string s;

	if ((line = readline(prompt)) == NULL)
		exit(0);
	add_history(line);
	s = string(line);
	free(line);
	if (s.back() == ';') {
		s.back() = ' ';
		s += ";";
	}
	for (int i = 0; i < s.length(); i++) {
		s[i] = tolower(s[i]);
	}
	return s;
}

int _rl_init(void)
{
	rl_bind_key('\n', _rl_bind_cr);
	rl_bind_key('\r', _rl_bind_cr);
	rl_bind_key('\t', rl_insert);
}

int _rl_bind_cr(int count, int key)
{
	int i;
	for (i = rl_end; i >= 0; i--)
		if (rl_line_buffer[i] == ';') {
			rl_done = 1;
			break;
		}
	if (i < 0)
		rl_insert_text("\n");
	else
		cout << endl;
}
