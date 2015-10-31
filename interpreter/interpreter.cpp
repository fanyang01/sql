//////////////////////////////////////////////////////////
///----------------------------------------------------///
///       Module: Interpreter_Module                   ///
///       Produced by: ZRJ                             ///
///       Description: Produced to deal with SQL parse ///
///       date: 2015/10                                ///
///----------------------------------------------------///
//////////////////////////////////////////////////////////
#include <iostream>
#include"Interpreter.h"
#include <cctype>
#include<cstdlib>
/////////////////////////////////////////////////////////////////////////////////////////////
// Read the user's input
string read_input()
{
	string sql;
	string tmp;
	
	bool finish = false;
	
	while(!finish)
	{
		cin >> tmp;
		sql += " " + tmp;
		if(sql[sql.length()-1] == ';')
		{
			sql[sql.length()-1] = ' ';
			sql += ";";
			finish = true;
		}
	}
	//Transfer sql to lower case.
	 for(int i = 0; i < sql.length(); i++)
	 {
	 	sql[i] = tolower(sql[i]);
	 }

	 return sql;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
//Get the attributes.
string get_attribute(string tmp, string sql)
{
	int start = 0, end, index, length;
	string T, C;
	tmp += " ";
	//Get the attribute name
	end = tmp.find(' ',start);
	T = tmp.substr(start, end - start);
	start = end + 1;
	sql += T + " ";
	//Get the attribute type
	while(tmp[start] == ' ')
		start++;
	end = tmp.length() - 1;
	T = tmp.substr(start, end-start);
	start = end + 1;
	length = T.length()-1;
	while(T[length] == ' ')
		length--;
	T = T.substr(0, length+1);
	//If empty
	if(T.empty())
	{
		cout << "error : error in create table statement!" << endl;
		sql = "99";
		return sql;
	}
	//int
	else if(T == "int")
		sql += "+";
	else if(T == "float")
		sql += "-";
	else
	{
		index = T.find('(');
		C = T.substr(0, index);
		if(C.empty())
		{
			cout << "error: " << T << "---is not a valid data type definition!" << endl;
			sql = "99";
			return sql;
		}
		//char
		else if(C == "char")
		{
			C = T.substr(index, T.length()-index-1);
			if(C.empty())
			{
				cout << "error: the length of the data type char has not been specified!" << endl;
				sql="99";
				return sql;
			}
			else 
				sql += C;
		}
		//If illegal
		else 
		{
			cout << "error: " << C << "---is not a valid key word!" << endl;
			sql = "99";
			return sql;
		}
	}
	//Whether or not exists addition information
	while(start < tmp.length() && tmp[start] == ' ')
		start++;
	if(start < tmp.length())
	{
		//If unique
		end = tmp.find(' ', start);
		T = tmp.substr(start, end - start);
		if(T == "unique")
			sql += " 1,";
		else 
		{
			cout << "error: " << T << "---is not a valid key word!"<<endl;
			sql="99";
			return sql;
		}
	}
	else 
		sql += " 0,";
	return sql;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the create_clause
string create_clause(string sql, int start)
{
	string tmp;
	int end;
	//Get the second key word
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//If it is empty
	if(tmp.empty())
	{
		cout << "syntax error: syntax error for create statement" << endl;
		sql = "99";
	}
//	else if(tmp == "database")
//		sql = create_database(sql, start);
	else if(tmp == "table")
		sql = create_table(sql, start);
	else if(tmp == "index")
		sql = create_index(sql, start);
	//If wrong
	else
	{
		cout << "syntax error:" << " " << tmp << "---is not a valid key word!"<<endl;
		sql == "99";
	}
	
	return sql;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//validate the create_database_clause
//(To be continue..)
///////////////////////////////////////////////////////////////////////////////////////////////////
//validate the create_table_clause
string create_table(string sql, int start)
{
	string tmp, T, SQL;
	int index, end, length;
	//Get the table name
	while(sql[start] == ' ')
		start++;
	index = start;
	if((end = sql.find('(', start)) == -1)
	{
		cout << "error: missing ( in the statement!" << endl;
		sql == "99";
		return sql;
	}
	tmp = sql.substr(start, end-start);
	start = end + 1;
	if(!tmp.empty())
	{
		while(sql[start] == ' ')
			start ++;
		length = tmp.length()-1;
		while(tmp[length] == ' ')
			length--;
		tmp = tmp.substr(0, length+1);
	}
	//If the name is empty
	if(tmp.empty())
	{
		cout << "error: error in create table statement!" << endl;
		sql == "99";
		return sql;
	}
	//If the name if illegal
	else if(tmp.find(' ') != -1)
	{
		cout<<"error: " << tmp << "---is not a valid table name!" << endl;
		sql="99";
		return sql;
	}
	else
	{
		SQL = tmp + ",";
		//Get each attribute
		while((end = sql.find(',', start)) != -1)
		{
			tmp = sql.substr(start, end-start);
			start = end + 1;
			if(tmp.empty())
			{
				cout << "error: error in create table statement!" << endl;
				sql="99";
				return sql;
			}
			else
			{
				SQL = get_attribute(tmp, SQL);
				if(SQL == "99")
					return SQL;
			}
			while(sql[start] == ' ')
				start++;
		}
		//Get the last attribute
		tmp = sql.substr(start, sql.length()-start-1);
		length = tmp.length() - 1;
		while(tmp[length] != ')' && length >= 0)
			length--;
		//If no last attribute
		if(length < 1)
		{
			cout << "error: error in create table statement!" << endl;
			sql="99";
			return sql;
		}
		//storage the attrbutes
		else
		{
			tmp = tmp.substr(0, length);
			end = sql.find(' ', start);
			T = sql.substr(start, end-start);
			start = end + 1;
			//If T is primary
			if(T == "primary")
			{
				//Determine if 'key' exist
				tmp += ")";
				while(sql[start] == ' ')
					start ++;
				end = sql.find('(', start);
				T = sql.substr(start, end - start);
				start = end + 1;
				length = T.length() - 1;
				while(T[length] == ' ')
					length--;
				T = T.substr(0, length+1);
				//If empty
				if(T.empty())
				{
					cout << "syntax error: syntax error in create table statement!" << endl;
					cout << "\t missing key word 'key' '!" << endl;
					sql="99";
					return sql;
				}
				else if(T == "key")
				{
					while(sql[start] == ' ')
						start++;
					end = sql.find(')', start);
					T = sql.substr(start, end-start);
					length = T.length() - 1;
					while(T[length] == ' ')
						length--;
					T = T.substr(0, length+1);
					//If not exist "key"
					if(T.empty())
					{
						cout << "error : missing primary key attribute name!" << endl;
						sql="99";
						return sql;
					}
					//If illegal
					else if(T.find(' ') != -1)
					{
						cout << "error : " << T << "---is not a valid primary key attribute name!" << endl;
						sql="99";
						return sql;
					}
					//Storage the primary key
					else 
					{
						SQL += T + " #;";
						sql = "01" + SQL;
					}
				}
				//If illegal
				else
				{
					cout << "error : " << T << "---is not a valid key word!" << endl;
					sql = "99";
					return sql;
				}
			}
			//if it is ordinary attribute
			else
			{
				SQL = get_attribute(tmp, SQL);
				if(SQL == "99")
				{
					sql ="99";
					return sql;
				}
				else
					sql = "01" + SQL;
			}
		}
	}
	
	return sql;
}
//////////////////////////////////////////////////////////////////////////////////////////////
//invalidate the create_index_on cluase
string create_index_on(string sql, int start, string SQL)
{
	string tmp;
	int end, length;
	//get the table name
	while(sql[start] == ' ')
		start++;
	end = sql.find('(', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//if not exist
	if(tmp.empty())
	{
		cout << "syntax error: syntax error for create index statement!" <<endl;
		cout << "\t missing ( !" << endl;
		sql = "99";
		return sql;
	}
	else
	{
		length = tmp.length() - 1;
		while(tmp[length] == ' ')
			length--;
		tmp = tmp.substr(0, length + 1);
		//if validated
		if(tmp.find(' ') == -1)
		{
			SQL += " " + tmp;
			//Get the attribute name
			while(sql[start] == ' ')
				start ++;
			end = sql.find(')', start);
			tmp = sql.substr(start, end-start);
			start = end + 1;
			//if name not exist
			if(tmp.empty())
			{
				cout << "syntax error: syntax error for create index statement!" << endl;
				cout << "\t missing ) !" << endl;
				sql = "99";
				return sql;
			}
			//if exists
			else
			{
				length = tmp.length() - 1;
				while(tmp[length] == ' ')
					length--;
				tmp = tmp.substr(0, length + 1);
				//if validated
				if(tmp.find(' ') == -1)
				{
					SQL += " " + tmp;
					while(sql[start] == ' ')
						start++;
					if(sql[start] != ';' || start != sql.length()-1)
					{
						cout << "syntax error: syntax error for quit!" << endl;
						sql = "99";
						return sql;
					}
					else 
						sql = "02" + SQL;
				}
			}
		}
	}
	
	return sql;
}

////////////////////////////////////////////////////////////////////////////////////////////
//invalidate the create_index clause
string create_index(string sql, int start)
{
	string tmp, SQL;
	int end;
	//Get the third word
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//If not exist
	if(tmp.empty())
	{
		cout << "syntax error: syntax error for create index statement!" << endl;
		sql = "99";
	}
	else
	{
		SQL = tmp;
		//get the forth word
		while(sql[start] == ' ')
			start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		//if not exists
		if(tmp.empty())
		{
			cout << "syntax error: syntax error for create index statement!" << endl;
			sql = "99";
		}
		//if do exist
		else if(tmp == "on")
			sql = create_index_on(sql, start, SQL);
		else
		{
			cout << "syntax error:" << " " << tmp << "---is not a valid key word!" << endl;
			sql = "99";
		}
	}
	
	return sql;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the drop_table_clause
string drop_table(string sql, int start)
{
	string tmp;
	int end, length;
	//get the table name
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//if name is empty
	if(tmp.empty())
	{
		cout << "error : table name has not been specified!" << endl;
		sql == "99";
		return sql;
	}
	else
	{
		while(sql[start] == ' ')
			start++;
		if(sql[start] != ';' || start != sql.length()-1)
		{
			cout << "error: "<< tmp <<" is not a valid name" << endl;
			sql == "99";
			return sql;
		}
		else
			sql = "11" + tmp;
	}
	
	return sql;
}
////////////////////////////////////////////////////////////////////////////////////////////
//Validate the drop_index_clause
string drop_index(string sql, int start)
{
	string tmp;
	int end, length;
	//get the index name
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//if name is empty
	if(tmp.empty())
	{
		cout << "error : index name has not been specified!" << endl;
		sql == "99";
		return sql;
	}
	else
	{
		while(sql[start] == ' ')
			start++;
		if(sql[start] != ';' || start != sql.length()-1)
		{
			cout << "error: "<< tmp <<" is not a valid name" << endl;
			sql == "99";
			return sql;
		}
		else
			sql = "11" + tmp;
	}
	
	return sql;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the drop_clause
string drop_clause(string sql, int start)
{
	string tmp;
	int end;
	//get the second word
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//if not exist
	if(tmp.empty())
	{
		cout << "syntax error: syntax error for drop statement!" << endl;
		sql = "99";
	}
	//for table
	else if(tmp == "table")
		sql = drop_table(sql, start);
	//for index
	else if(tmp == "index")
		sql = drop_index(sql, start);
	else
	{
		cout << "syntax error:" << " " << tmp << "---is not a valid key word!" << endl;
		sql = "99";
	}
	
	return sql;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the select_with_where
string select_with_where(string sql, int start, string SQL)
{
	string tmp;
	int end, length, index;
	//find and
	while((index = sql.find("and",start)) != -1)
	{
		//get the attribute name
		while(sql[start] == ' ')
			start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;	
		if(tmp.empty() || tmp == "and")
		{
			cout << "error: no attribute name found!" << endl;
			sql = "99";
			return sql;
		}
		else
			SQL += "," + tmp + " ";
		//get the sign
		while(sql[start] == ' ')
			start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(tmp.empty() || (tmp !="=" && tmp !=">" && tmp !="<" && tmp !=">="&& tmp !="<="&& tmp !="<>"))
		{
			cout << "error: no sign found!" << endl;
			sql = "99";
			return sql;
		}
		else
			SQL += tmp + " ";
		//get the value
		while(sql[start] == ' ')
			start++;
		end = sql.find(" ", start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(tmp.empty()|| tmp == "and")
		{
			cout << "error: no value found!" << endl;
			sql = "99";
			return sql;
		}
		else
			SQL += tmp;
		//validate whether sth followed
		while(sql[start] == ' ')
			start++;
		end = sql.find("and", end);
		tmp = sql.substr(start, end-start);
		length = tmp.length() - 1;
		while(tmp[length] == ' ')
			length--;
		tmp = tmp.substr(0, length + 1);
		if(!tmp.empty())
		{
			cout << "syntax error : illegal expression before 'and'!" << endl;
			sql = "99";
			return sql;
		}
		//cross "and"
		start = sql.find("and", start);
		start = sql.find(' ', start);
	}
	//find the last contition
	//get the attribute name
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;	
	if(tmp.empty())
	{
		cout << "error: no attribute name found!" << endl;
		sql = "99";
		return sql;
	}
	else
		SQL += "," + tmp + " ";
	//get the sign
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	if(tmp.empty() || (tmp !="=" && tmp !=">" && tmp !="<" && tmp !=">="&& tmp !="<="&& tmp !="<>"))
	{
		cout << "error: no sign found!" << endl;
		sql = "99";
		return sql;
	}
	else
		SQL += tmp + " ";
	//get the value
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = sql.find(' ', end);	
	if(tmp.empty())
	{
		cout << "error: no value found!" << endl;
		sql = "99";
		return sql;
	}
	else
		SQL += tmp + ";";
	//validate whether sth followed
	while(sql[start] == ' ')
		start++;
	end = sql.find(';', start);
	tmp = sql.substr(start, end-start);
	length = tmp.length() - 1;
	while(tmp[length] == ' ')
		length--;
	tmp = tmp.substr(0, length + 1);
	if(!tmp.empty())
	{
		cout << "syntax error : illegal expression before ';'" << endl;
		sql = "99";
		return sql;
	}
	//if validated
	sql = "21" + SQL;
	
	return sql;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the select_cluase
string select_clause(string sql,int start)
{
	string tmp, SQL;
	int end, length;
	//get the second word"*"
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	if(tmp != "*")
	{
		cout << "error :  attributes are not specified!" << endl;
		sql = "99";
		return sql;
	}
	else
	{
		//get the third word "from"
		while(sql[start] == ' ')
		start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		//if not exists
		if(tmp.empty())
		{
			cout << "syntax error for 'from statement!" << endl;
			sql = "99";
			return sql;
		}
		else if(tmp != "from")
		{
			cout << "syntax error: " << tmp << " is not a valid key word!" << endl;
			sql = "99";
			return sql;
		}
		else
		{
			//get the forth word table name
			while(sql[start] == ' ')
				start++;
			end = sql.find(' ', start);
			tmp = sql.substr(start, end-start);
			start = end + 1;
			if(tmp.empty())
			{
				cout << "syntax error for table name!" << endl;
				sql = "99";
				return sql;
			}
			else if(tmp.find(' ') != -1)
			{
				cout << "error: "<< tmp <<" is not a valid name" << endl;
				sql = "99";
				return sql;
			}
			else
			{
				//get the next word"where"
				SQL = tmp ;
				while(sql[start] == ' ')
					start++;
				end = sql.find(' ', start);
				tmp = sql.substr(start, end-start);
				start = end + 1;
				//no "where"
				if(start == 0)
				{
					sql = "20" + SQL;
					return sql;
				}
				//"where" exist
				else if(tmp == "where")
				{
					sql = select_with_where(sql, start, SQL);
				}
				//syntax error
				else
				{
					cout << "syntax error for where statement " << endl;
					sql = "99";
					return sql;
				}
			}
		}
	}
	
	return sql;
	
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
/////////////////////////////////////////////////////////////////////////////////////////////////
//validate the insert_into_values_clause
string insert_into_values(string sql,int start,string SQL)
{
	string tmp;
	int end, length;
	//find '('
	end = sql.find('(', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	// find ','
	while((end = sql.find(',', start)) != -1)
	{
		// get the value
		while(sql[start] == ' ')
			start++;
		tmp = sql.substr(start, end-start);
		start = end + 1;
		length = tmp.length() - 1;
		while(tmp[length] == ' ')
			length--;
		tmp = tmp.substr(0, length + 1);
		//validate the value
		if(tmp.empty())
		{
			cout << "error : value can't be empty!" << endl;
			sql == "99";
			return sql;
		}
		else if(isInt(tmp) || isFloat(tmp) || isString(tmp))
		{
			SQL += tmp + ",";
		}
		else 
		{
			cout << "error : invalidated value!" << endl;
			sql == "99";
			return sql;
		}
	}
	// find the last value;
	end = sql.find(')', start);
	while(sql[start] == ' ')
		start++;
	tmp = sql.substr(start, end-start);
	start = end + 1;
	length = tmp.length() - 1;
	while(tmp[length] == ' ')
		length--;
	tmp = tmp.substr(0, length + 1);
	//validate
	if(tmp.empty())
	{
		cout << "error : value can't be empty!" << endl;
		sql == "99";
		return sql;
	}
	else if(isInt(tmp) || isFloat(tmp) || isString(tmp))
	{
		SQL += tmp + ";";
	}
	else 
	{
			cout << "error : invalidated value!" << endl;
		sql == "99";
		return sql;
	}
	sql = "30" + SQL;
	
	return sql;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the insert_clause
string insert_clause(string sql, int start)
{
	string SQL, tmp;
	int end, length;
	//find "into"
	while(sql[start] == ' ')
			start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//if "into" not exist
	if(tmp.empty())
	{
		cout << "syntax error for 'into statement!" << endl;
		sql = "99";
		return sql;
	}
	else if(tmp != "into")
	{
		cout << "syntax error: " << tmp << " is not a valid key word!" << endl;
		sql = "99";
		return sql;
	}
	//get the table name
	else
	{
		while(sql[start] == ' ')
			start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		//if table name is empty
		if(tmp.empty())
		{
			cout << "syntax error for table name!" << endl;
				sql = "99";
			return sql;
		}
		SQL = tmp + ",";
		while(sql[start] == ' ')
				start++;
		end = sql.find('(', start);
		tmp = sql.substr(start, end-start);
		start = end;
		length = tmp.length() - 1;
		while(tmp[length] == ' ')
			length--;
		tmp = tmp.substr(0, length + 1);
		//if "values" not exist
		if(tmp.empty())
		{
			cout << "syntax error for 'values statement!" << endl;
			sql = "99";
			return sql;
		}
		else if(tmp != "values")
		{
			cout << "syntax error: " << tmp << " is not a valid key word!" << endl;
			sql = "99";
			return sql;
		}
		while(sql[start] == ' ')
				start++;
		sql = insert_into_values(sql, start, SQL);
	}
	
	return sql;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the delete_with_where
string delete_with_where(string sql, int start, string SQL)
{
	string tmp;
	int end, length, index;
	//find and
	while((index = sql.find("and",start)) != -1)
	{
		//get the attribute name
		while(sql[start] == ' ')
			start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;	
		if(tmp.empty() || tmp == "and")
		{
			cout << "error: no attribute name found!" << endl;
			sql = "99";
			return sql;
		}
		else
			SQL += "," + tmp + " ";
		//get the sign
		while(sql[start] == ' ')
			start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(tmp.empty() || (tmp !="=" && tmp !=">" && tmp !="<" && tmp !=">="&& tmp !="<="&& tmp !="<>"))
		{
			cout << "error: no sign found!" << endl;
			sql = "99";
			return sql;
		}
		else
			SQL += tmp + " ";
		//get the value
		while(sql[start] == ' ')
			start++;
		end = sql.find(" ", start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(tmp.empty()|| tmp == "and")
		{
			cout << "error: no value found!" << endl;
			sql = "99";
			return sql;
		}
		else
			SQL += tmp;
		//validate whether sth followed
		while(sql[start] == ' ')
			start++;
		end = sql.find("and", end);
		tmp = sql.substr(start, end-start);
		length = tmp.length() - 1;
		while(tmp[length] == ' ')
			length--;
		tmp = tmp.substr(0, length + 1);
		if(!tmp.empty())
		{
			cout << "syntax error : illegal expression before 'and'!" << endl;
			sql = "99";
			return sql;
		}
		//cross "and"
		start = sql.find("and", start);
		start = sql.find(' ', start);
	}
	//find the last contition
	//get the attribute name
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;	
	if(tmp.empty())
	{
		cout << "error: no attribute name found!" << endl;
		sql = "99";
		return sql;
	}
	else
		SQL += "," + tmp + " ";
	//get the sign
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	if(tmp.empty() || (tmp !="=" && tmp !=">" && tmp !="<" && tmp !=">="&& tmp !="<="&& tmp !="<>"))
	{
		cout << "error: no sign found!" << endl;
		sql = "99";
		return sql;
	}
	else
		SQL += tmp + " ";
	//get the value
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = sql.find(' ', end);	
	if(tmp.empty())
	{
		cout << "error: no value found!" << endl;
		sql = "99";
		return sql;
	}
	else
		SQL += tmp + ";";
	//validate whether sth followed
	while(sql[start] == ' ')
		start++;
	end = sql.find(';', start);
	tmp = sql.substr(start, end-start);
	length = tmp.length() - 1;
	while(tmp[length] == ' ')
		length--;
	tmp = tmp.substr(0, length + 1);
	if(!tmp.empty())
	{
		cout << "syntax error : illegal expression before ';'" << endl;
		sql = "99";
		return sql;
	}
	//if validated
	sql = "21" + SQL;
	
	return sql;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//Validate the delete_cluase
string delete_clause(string sql,int start)
{
	string tmp, SQL;
	int end, length;
	//get the second word"*"
//	while(sql[start] == ' ')
//		start++;
//	end = sql.find(' ', start);
//	tmp = sql.substr(start, end-start);
//	start = end + 1;
//	if(tmp != "*")
//	{
//		cout << "error :  attributes are not specified!" << endl;
//		sql = "99";
//		return sql;
//	}
//	else
//	{
	//get the third word "from"
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//if not exists
	if(tmp.empty())
	{
		cout << "syntax error for 'from statement!" << endl;
		sql = "99";
		return sql;
	}
	else if(tmp != "from")
	{
		cout << "syntax error: " << tmp << " is not a valid key word!" << endl;
		sql = "99";
		return sql;
	}
	else
	{
		//get the forth word table name
		while(sql[start] == ' ')
			start++;
		end = sql.find(' ', start);
		tmp = sql.substr(start, end-start);
		start = end + 1;
		if(tmp.empty())
		{
			cout << "syntax error for table name!" << endl;
				sql = "99";
			return sql;
		}
		else if(tmp.find(' ') != -1)
		{
			cout << "error: "<< tmp <<" is not a valid name" << endl;
			sql = "99";
			return sql;
		}
			else
		{
			//get the next word"where"
			SQL = tmp ;
			while(sql[start] == ' ')
				start++;
			end = sql.find(' ', start);
			tmp = sql.substr(start, end-start);
			start = end + 1;
			//no "where"
			if(start == 0)
			{
				sql = "20" + SQL;
				return sql;
			}
			//"where" exist
			else if(tmp == "where")
			{
				sql = select_with_where(sql, start, SQL);
			}
			//syntax error
			else
			{
				cout << "syntax error for where statement " << endl;
				sql = "99";
				return sql;
			}
		}
	}
//}
	
	return sql;
	
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get the user's input and switch the key word to specific function
string Interpreter(string statement)
{
	string sql;
	string tmp;
	int start = 0, end;
	
	if(statement.empty())
		sql = read_input();
	else
		sql = statement;
	//Get the first word in sql
	while(sql[start] == ' ')
		start++;
	end = sql.find(' ', start);
	tmp = sql.substr(start, end-start);
	start = end + 1;
	//If it is empty:
	if(tmp.empty())
	{
		cout << "syntax error : empty statement!" << endl;
		sql = "99";
	}
	//create_database 00, create_table 01, create_index 02
	else if(tmp == "create")
		sql = create_clause(sql, start);
	//drop_database 10, drop_table 11, drop index 12;
	else if(tmp == "drop")
		sql = drop_clause(sql, start);
	//select without where 20, select_with_where 21;
	else if(tmp == "select")
		sql = select_clause(sql, start);
	//insert 30
	else if(tmp == "insert")
		sql = insert_clause(sql, start);
//	//delete without where 40, delete with where 41
	else if(tmp == "delete")
		sql = delete_clause(sql, start);
//	//use 50
//	else if(tmp == "use")
//		sql = use_clause(sql, start);
	//quit 60
//	else if(tmp == "quit")
//		sql = quit_clause(sql, start);
	//execute_file 70
//	else if(tmp == "execfile")
//		sql = execfile_clause(sql, start);
	//help 80
	else if(tmp == "help")
		sql = "80";
	//If illigal
	else
	{
		cout << "syntax error: " << tmp << " is not a valid key word!" << endl;
		sql = "99";
	}
	
	return sql;
}

