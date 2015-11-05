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
extern "C" {
#include <histedit.h>
}

using namespace std;

const string welcome = R"(
  ********   *******    **      
 **//////   **/////**  /**      
/**        **     //** /**      
/*********/**      /** /**      
////////**/**    **/** /**      
       /**//**  // **  /**      
 ********  //******* **/********
////////    /////// // //////// 
)";

const char *TMP_FILE = "/tmp/tmp.db";

DB *db;
string(*readsql) ();
static string _readline();

static inline string & ltrim(string & s);
static inline string & rtrim(string & s);
static inline string & trim(string & s);

void docmd(string line);
void exec_sqlfile(string filename);

static EditLine *el;
History *hist;
HistEvent ev;

const char *prompt(EditLine *e)
{
	return "OURSQL=>>> ";
}

static void init_editline(char *argv[])
{
	el = el_init(argv[0], stdin, stdout, stderr);
	el_set(el, EL_PROMPT, &prompt);
	el_set(el, EL_EDITOR, "emacs");

	if((hist = history_init()) == NULL) {
		cerr << "failed to init history" << endl;
		exit(1);
	}

	history(hist, &ev, H_SETSIZE, 800);

	el_set(el, EL_HIST, history, hist);
}

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
		readsql = _readline;
		init_editline(argv);
		cout << "\033[1;31m" << welcome << "\033[0m" << endl;
		cout << "Welcome to OURSQL! type 'help' to show help messages."
			<< endl << endl;
	} else
		readsql = read_input;

	while (true) {
		API_Module(Interpreter(readsql()));
	}
}

string _readline()
{
	const char *line;
	string s;
	int count;

	line = el_gets(el, &count);

	if (count <= 0)
		return _readline();

	history(hist, &ev, H_ENTER, line);

	s = string(line);
	s = trim(s);

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
