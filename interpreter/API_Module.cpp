#include"API_Module.h"
#include <cstring>
#include <cstdlib>
//#include"Catalog_Manager.h"
//#include"Record_Manager.h"
//#include"Index_Manager.h"
using namespace std;


//////////////////////////////////////////////////////////////////
//struct the create_table
int count_attribute(string sql)
{
	int num = 0;
	
	for(int i = 0; i < sql.length(); i++)
	{
		if(sql[i] == ',')
			num++;
	}
	
	return num-1;
}

stmt_t* create_table_struct(string sql, stmt_t *state)
{
	int end, count_attr, start = 2;
	string table_name, tmp; 
	
	state->type = STMT_CREAT_TABLE;
	//table name
	end = sql.find(',', start);
	table_name = sql.substr(start, end - start);
	start = end+1;
	strcpy(state->table, table_name.c_str());
	//the number of attribute
		count_attr = count_attribute(sql);
	state->ncol = count_attr;
	//attributes
	state->cols = new col_t[count_attr];
	for(int i = 0; i < count_attr; i++)
	{
		//find the attribute name
		end = sql.find(' ', start);
		tmp = sql.substr(start, end - start);
		start = end + 1;
		strcpy(state->cols[i].name, tmp.c_str());
		//find the type
		end = sql.find(' ', start);
		tmp = sql.substr(start, end - start);
		if(tmp == "+")
			state->cols[i].type = TYPE_INT;
		else if(tmp == "-")
			state->cols[i].type = TYPE_FLOAT;
		else
		{
			state->cols[i].type = TYPE_STRING;
			tmp = sql.substr(start+1, end -start-1);
			state->cols[i].size = atoi(tmp.c_str());
		}
		//find the unique sign
		start = end + 1;
		if(sql[start] == '1')
			state->cols[i].unique = COL_UNIQUE;
		else 
			state->cols[i].unique = COL_NORMAL;
		start += 2;
	}
	//find the primary key
	end = sql.find(' ', start);
	tmp = sql.substr(start, end - start);
	for(int i = 0; i < count_attr; i++)
	{
		if(strcmp(state->cols[i].name, tmp.c_str()) == 0)
		{
			state->cols[i].unique = COL_PRIMARY;
			break;
		}
	}
	
	return state;
}
//////////////////////////////////////////////////////////////////
//struct the create_index
//02in1,table,attr;
stmt_t* create_index_struct(string sql, stmt_t *state)
{
	int end, count_attr, start = 2;
	string table_name, tmp; 
	string index_name, attr_name;
	
	state->type = STMT_CREAT_INDEX;
	//index name
	end = sql.find(',', start);
	index_name = sql.substr(start, end - start);
	start = end+1;
	strcpy(state->index, index_name.c_str());
	//table_name
	end = sql.find(',', start);
	table_name = sql.substr(start, end - start);
	start = end + 1;
	strcpy(state->table, table_name.c_str());
	//attribute name
	end = sql.find(';', start);
	attr_name = sql.substr(start, end - start);
	strcpy(state->attr, attr_name.c_str());
	
	return state;
}
//////////////////////////////////////////////////////////////////
//struct the drop_table
//20table;
stmt_t* drop_table_struct(string sql, stmt_t *state)
{
	int end, count_attr, start = 2;
	string table_name, tmp; 
	
	state->type = STMT_DROP_TABLE;
	//table name
	end = sql.find(';', start);
	table_name = sql.substr(start, end - start);
	strcpy(state->table, table_name.c_str());
	
	return state;
}
//////////////////////////////////////////////////////////////////
//struct the drop_index
stmt_t* drop_index_struct(string sql, stmt_t *state)
{
	int end, count_attr, start = 2;
	string index_name, tmp; 
	
	state->type = STMT_DROP_INDEX;
	//table name
	end = sql.find(';', start);
	index_name = sql.substr(start, end - start);
	strcpy(state->index, index_name.c_str());
	
	return state;
}
//////////////////////////////////////////////////////////////////
//struct the select_without
//20table;
stmt_t* select_without_struct(string sql, stmt_t *state)
{
	int end, count_cond, start = 2, count_select = 0;
	string table_name, tmp, attr; 
	
	state->type = STMT_SELECT;
	//table name
	end = sql.find(',', start);
	table_name = sql.substr(start, end - start);
	strcpy(state->table, table_name.c_str());
	/////////////////////////////////////////
	state->cols = new col_t[MAXCOLS];
	start = end + 2;
//	end = sql.find(' ', start);
//	attr = sql.substr(start, end);
//	start = end + 1;
	while(1)
	{
		end = sql.find(' ', start);
		attr = sql.substr(start, end - start);
		start = end + 1;
		if(attr == ";") 
			break;
		strcpy(state->cols[count_select].name, attr.c_str());
		count_select ++ ;
	}
	state->ncol = count_select;

	
	return state;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the value for insert
static bool isInt(string tmp)
{
    char *end, c[100];
    int n;
  
	strcpy(c, tmp.c_str());
	n = strtol(c, &end, 10);
	//cout << strtol(c, &end, 10) << endl;
	if(*end == 0 && n != 0)
		return true;
	else if(*end ==0 && n == 0 && c[0] == '0')
		return true;
	else
		return false;
}
//for double
static bool isFloat(string tmp)
{
    char *end, c[100];
    float n;
  
  	if(tmp.find('.') == -1)
  		return false;
	strcpy(c, tmp.c_str());
	n = strtod(c, &end);
	//cout << strtol(c, &end, 10) << endl;
	if(*end == 0 && n != 0)
		return true;
	else if(*end ==0 && n == 0 && c[0] == '0')
		return true;
	else
		return false;
}
//for string
static bool isString(string tmp)
{
	if(tmp[0] == '\'' && tmp[tmp.length()-1] == '\'')
		return true;
	else
		return false;
}
//////////////////////////////////////////////////////////////////
//struct the select_with
int count_condition(string sql)
{
	int num = 0;
	
	for(int i = 0; i < sql.length(); i++)
	{
		if(sql[i] == ',')
			num++;
	}
	
	return num - 1;
}
//21student, sage > 20,sgender = 'f';
//21author, a_name id ; ,a_id > 35;
//21author, ; ,a_name < 'naae';
stmt_t* select_with_struct(string sql, stmt_t *state)
{
	int end, count_cond, start = 2, count_select = 0;
	string table_name, tmp, attr; 
	
	state->type = STMT_SELECT;
	//table name
	end = sql.find(',', start);
	table_name = sql.substr(start, end - start);
	strcpy(state->table, table_name.c_str());
	/////////////////////////////////////////
	state->cols = new col_t[MAXCOLS];
	start = end + 2;
//	end = sql.find(' ', start);
//	attr = sql.substr(start, end);
//	start = end + 1;
	while(1)
	{
		end = sql.find(' ', start);
		attr = sql.substr(start, end - start);
		start = end + 1;
		if(attr == ";") 
			break;
		strcpy(state->cols[count_select].name, attr.c_str());
		count_select ++ ;
	}
	state->ncol = count_select;
	start ++;
	/////////////////////////////
	//count conditions
	count_cond = count_condition(sql);
	state->ncond = count_cond;
	state->conds = new cond_t[count_cond];
	//get the conditions
	for(int i = 0; i < count_cond; i++)
	{
		//get attribute name
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		strcpy(state->conds[i].attr, tmp.c_str());
		//get op
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(tmp == "=")
			state->conds[i].op = OP_EQ;
		else if(tmp == "<>")
			state->conds[i].op = OP_NEQ;
		else if(tmp == ">")
			state->conds[i].op = OP_GT;
		else if(tmp == ">=")
			state->conds[i].op = OP_GE;
		else if(tmp == "<")
			state->conds[i].op = OP_LT;
		else if(tmp == "<=")
			state->conds[i].op = OP_LE;
		//get value
		if((end = sql.find(',', start) )== -1)
			end = sql.length()-1;
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(isInt(tmp))
		{
			state->conds[i].operand.type = TYPE_INT;
			state->conds[i].operand.value.i = atoi(tmp.c_str());
		}
		else if(isFloat(tmp))
		{
			state->conds[i].operand.type = TYPE_FLOAT;
			state->conds[i].operand.value.f = atof(tmp.c_str());
		}
		else if(isString(tmp))
		{
			state->conds[i].operand.type = TYPE_STRING;
			state->conds[i].operand.value.s = new char[STRING_MAX];
			strcpy(state->conds[i].operand.value.s, tmp.substr(1, tmp.length()-2).c_str());
		}
	}
	
	return state;
}
//////////////////////////////////////////////////////////////////
//struct the delect_without
stmt_t* delect_without_struct(string sql, stmt_t *state)
{
	int end, count_attr, start = 2;
	string table_name, tmp; 
	
	state->type = STMT_DELETE;
	//table name
	end = sql.find(';', start);
	table_name = sql.substr(start, end - start);
	strcpy(state->table, table_name.c_str());
	
	state->conds = NULL;
	
	return state;
}
//////////////////////////////////////////////////////////////////
//struct the delect_with
stmt_t* delect_with_struct(string sql, stmt_t *state)
{
	int end, count_cond, start = 2;
	string table_name, tmp; 
	
	state->type = STMT_DELETE;
	//table name
	end = sql.find(',', start);
	table_name = sql.substr(start, end - start);
	strcpy(state->table, table_name.c_str());
	//count
	count_cond = count_condition(sql);
	state->ncond = count_cond;
	state->conds = new cond_t[count_cond];
	//get the conditions
	start = end + 1;
	for(int i = 0; i < count_cond; i++)
	{
		//get attribute name
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		strcpy(state->conds[i].attr, tmp.c_str());
		//get op
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(tmp == "=")
			state->conds[i].op = OP_EQ;
		else if(tmp == "<>")
			state->conds[i].op = OP_NEQ;
		else if(tmp == ">")
			state->conds[i].op = OP_GT;
		else if(tmp == ">=")
			state->conds[i].op = OP_GE;
		else if(tmp == "<")
			state->conds[i].op = OP_LT;
		else if(tmp == "<=")
			state->conds[i].op = OP_LE;
		//get value
		if((end = sql.find(',', start) )== -1)
			end = sql.length()-1;
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(isInt(tmp))
		{
			state->conds[i].operand.type = TYPE_INT;
			state->conds[i].operand.value.i = atoi(tmp.c_str());
		}
		else if(isFloat(tmp))
		{
			state->conds[i].operand.type = TYPE_FLOAT;
			state->conds[i].operand.value.f = atof(tmp.c_str());
		}
		else if(isString(tmp))
		{
			state->conds[i].operand.type = TYPE_STRING;
			state->conds[i].operand.value.s = new char[STRING_MAX];
			strcpy(state->conds[i].operand.value.s, tmp.substr(1, tmp.length()-2).c_str());
		}
	}
	
	return state;
}
//////////////////////////////////////////////////////////////////
//struct the insert
int count_value(string sql)
{
	int num = 0;
	for(int i = 0; i < sql.length(); i++)
	{
		if(sql[i] == ',')
			num++;
	}
	
	return num;
}
//30student,'12345678','wy',22,'m';
stmt_t* insert_struct(string sql, stmt_t *state)
{
	int end, count_val, start = 2;
	string table_name, tmp; 
	
	state->type = STMT_INSERT;
	//table name
	end = sql.find(',', start);
	table_name = sql.substr(start, end - start);
	start = end+1;
	strcpy(state->table, table_name.c_str());
	count_val = count_value(sql);
	state->nval = count_val;
	state->vals = new colv_t[count_val];
	for(int i = 0; i < count_val; i++)
	{
		if((end = sql.find(',', start) )== -1)
			end = sql.length()-1;
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(isInt(tmp))
		{
			state->vals[i].type = TYPE_INT;
			state->vals[i].value.i = atoi(tmp.c_str());
		}
		else if(isFloat(tmp))
		{
			state->vals[i].type = TYPE_FLOAT;
			state->vals[i].value.f = atof(tmp.c_str());
		}
		else if(isString(tmp))
		{
			state->vals[i].type = TYPE_STRING;
			state->vals[i].value.s = new char[STRING_MAX];
			strcpy(state->vals[i].value.s, tmp.substr(1, tmp.length()-2).c_str());
		}
	}
	
	return state;
}
///////////////////////////////////////////////////////////////////////
//help
void help()
{
	
}
/////////////////////////////////////////////////////////////////////
//quit
void quit()
{
	exit(0);
}
/////////////////////////////////////////////////////////////////////
//API
void API_Module(string sql)
{
//	stmt_t SQL;
	string type = sql.substr(0,2);
	string table_name, attr_name, index_name, tmp;
	int start, end;
	int count_attr;
	stmt_t *state = new stmt_t();
	//Wrong SQL 
	if(type == "99")
		return ;
	//create table
	else if(type == "01")
	{
		create_table_struct(sql, state);
		//Create_Table(state);
	}
	//create index
	else if(type == "02")
	{
		create_index_struct(sql, state);
	}
	//drop table
	else if(type == "11")
	{
		drop_table_struct(sql, state);
	}
	//drop index
	else if(type == "12")
	{
		drop_index_struct(sql, state);
	}
	//select without where

	else if(type == "20")
	{
		select_without_struct(sql, state);
	}
	//select with where
	else if(type == "21")
	{
		select_with_struct(sql, state);
	}
	//insert 30
	//30student,'12345678','wy',22,'m';
	else if(type == "30")
	{
		insert_struct(sql, state);
	}
	//delete without where
	//40student
	else if(type == "40")
	{
		delect_without_struct(sql, state);
	}
	//delete with where
	//41t1,salary > 4000.00,name <> 'jim';
	else if(type == "41")
	{
		delect_with_struct(sql, state);
	}
	//help
	else if(type == "80")
	{
		help();
	}
	//quit
	else if(type == "60")
	{
		quit();
	}
	
	delete state;
}
