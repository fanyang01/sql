//////////////////////////////////////////////////////////
///----------------------------------------------------///
///       Module: Interpreter                          ///
///       Produced by: Zhang Renjie                    ///
///       Description: Read user's input and change it ///
///                    to another format that can be   ///
///                    recognized by Module API        ///
///       date: 2015/10                                ///
///----------------------------------------------------///
//////////////////////////////////////////////////////////
#ifndef _INTERPRETER_H
#define _INTERPRETER_H

using namespace std;
#include "MySQL.h"
#include <string>
//interpreter the use's input into internal form
string Interpreter(string statement);
//read the use's input 
string read_input();
//create table or index
string create_clause(string SQL,int start);
////create database
//string create_database(string SQL,int start);
//create table
string create_table(string SQL,int start);

//get the attribute
string get_attribute(string temp,string sql);
//create index
string create_index(string SQL,int start);
//create index on an attribut
string create_index_on(string SQL,int start,string sql);
//drop a databse, table or index
string drop_clause(string SQL,int start);
//drop database
string drop_database(string SQL,int start);
//drop table
string drop_table(string SQl,int start);
//drop index
string drop_index(string SQL,int start);
//select with where or without where
string select_clause(string SQL,int start);
//select with where
string select_with_where(string sql, int start, string SQL);
//
string get_part(string temp,string sql,string kind);
//insert a tuple
string insert_clause(string SQL,int start);
//insert into with values
string insert_into_values(string SQL,int start,string sql);
//delete with where or without where
string delete_clause(string SQL,int start);
//delete with where
string delete_from_where(string SQL,int start,string sql);
//
//string get_expression(string temp,string sql);
////
//string get_each(string T,string sql,string condition);
////
//string use_clause(string SQL,int start);
//
string execfile_clause(string SQL,int start);
//
string quit_clause(string SQL,int start);

#endif
