#include "db.h"
#include "API_Module.h"
#include "Interpreter.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <unistd.h>
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;

const char *TMP_FILE = "/tmp/tmp.db";
const char *prompt = "\033[1;36moursql=>\033[0m ";
DB *db;
string(*readsql) ();
static string _readline();

// readline
int _rl_init(void);
int _rl_bind_cr(int count, int key);

// autocomplete
static char **my_completion(const char *, int, int);
static char *my_generator(const char *, int);
static char *dupstr(const char *s, const char *start, const char *word);
static const char *last_word(const char *s);
static char *xmalloc(int);

static const char *cmd[] = {
	"create", "table", "index", "on",
	"insert", "into", "values",
	"delete", "from",
	"select", "where", "and",
	"drop",
	"primary", "key",
	NULL,
};

static inline string & ltrim(string & s);
static inline string & rtrim(string & s);
static inline string & trim(string & s);

void docmd(string line);
void exec_sqlfile(string filename);

int main(int argc, char *argv[])
{
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

	if (strlen(line) == 0) {
		free(line);
		return _readline();
	}
	add_history(line);
	s = string(line);
	s = trim(s);
	free(line);

	if (s[0] == '\\') {
		/* s.erase(s.length() - 1, 1); */
		docmd(s);
		return _readline();
	}
	if (s[s.length() - 1] == ';') {
		s[s.length() - 1] = ' ';
		s += ";";
	}
	for (int i = 0; i < s.length(); i++) {
		s[i] = tolower(s[i]);
	}
	return s;
}

void docmd(string line)
{
	line.erase(0, 1);
	if (line == "")
		return;
	switch (line[0]) {
	case 'd':
		line.erase(0, 1);
		if (line == "") {
			cout << "TABLES" << endl;
			show_tables(db);
			cout << endl;
			cout << "INDICES" << endl;
			show_indices(db);
			cout << endl;
			return;
		}
		switch (line[0]) {
		case 't':
			cout << "TABLES" << endl;
			show_tables(db);
			break;
		case 'i':
			cout << "INDICES" << endl;
			show_indices(db);
			break;
		default:
			cerr << "Unsupported command: " << "\\d" << line <<
			    endl;
			break;
		}
		break;
	case 'i':
		exec_sqlfile(line.erase(0, 1));
		break;
	default:
		cerr << "Unsupported command: " << "\\" << line << endl;
		break;
	}
}

void exec_sqlfile(string filename)
{
}

int _rl_init(void)
{
	rl_attempted_completion_function = my_completion;
	using_history();
	stifle_history(50);
	rl_bind_key('\n', _rl_bind_cr);
	rl_bind_key('\r', _rl_bind_cr);
	rl_bind_key('\t', rl_complete);
}

int _rl_bind_cr(int count, int key)
{
	int i;

	if (rl_end > 0 && rl_line_buffer[0] == '\\') {
		cout << endl;
		rl_done = 1;
		return 0;
	}
	for (i = rl_end; i >= 0; i--)
		if (rl_line_buffer[i] == ';') {
			rl_done = 1;
			break;
		}
	if (i < 0)
		rl_insert_text("\n");
	else
		cout << endl;
	return 0;
}

// trim from start
string & ltrim(string & s)
{
	s.erase(s.begin(),
		find_if(s.begin(), s.end(),
			not1(ptr_fun < int, int >(isspace))));
	return s;
}

// trim from end
string & rtrim(string & s)
{
	s.erase(find_if(s.rbegin(), s.rend(),
			not1(ptr_fun < int, int >(isspace))).base(), s.end());
	return s;
}

// trim from both ends
string & trim(string & s)
{
	return ltrim(rtrim(s));
}

// http://cc.byexamples.com/2008/06/16/gnu-readline-implement-custom-auto-complete/
static char **my_completion(const char *text, int start, int end)
{
	char **matches;

	matches = (char **)NULL;

	matches = rl_completion_matches((char *)text, &my_generator);

	return (matches);

}

char *my_generator(const char *text, int state)
{
	static int list_index, len;
	const char *name, *lastword;

	if (!state) {
		list_index = 0;
		len = strlen(text);
	}

	lastword = last_word(text);
	while (name = cmd[list_index]) {
		list_index++;

		if (strncmp(name, lastword, len - (lastword - text)) == 0)
			return (dupstr(text, lastword, name));
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);

}

char *dupstr(const char *s, const char *start, const char *word)
{
	char *r;
	int len1 = start - s;
	int len2 = strlen(word);
	int len = len1 + len2;

	r = (char *)xmalloc(len + 1);
	for (int i = 0; i < len1; i++)
		r[i] = s[i];
	for (int i = len1; i < len; i++)
		r[i] = toupper(word[i]);
	r[len] = 0;
	return r;
}

char *xmalloc(int size)
{
	char *buf;

	buf = (char *)malloc(size);
	if (!buf) {
		fprintf(stderr, "Error: Out of memory\n");
		exit(1);
	}

	return buf;
}

const char *last_word(const char *s)
{
	int len = strlen(s);
	const char *p = s + len - 1;

	while (isspace(*p) && p > s)
		p--;
	for (; p > s; p--)
		if (isspace(*p)) {
			p++;
			break;
		}
	return p;
}
