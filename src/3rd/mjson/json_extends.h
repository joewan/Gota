#ifndef JSONG_EXTENDS_H__
#define JSONG_EXTENDS_H__

#ifdef __cplusplus
extern "C"
{
#include "json.h"
#endif

void 
json_free_str(char* json_str);

int
json_object_size(json_t* parent);

int
json_array_size(json_t* parent);

json_t* 
json_object_at(json_t* parent, char* key);

json_t*
json_array_at(json_t* parent, int index);

void
json_object_push_number(json_t* parent, char* key, int value);

void
json_object_push_string(json_t* parent, char* key, char* value);

void
json_array_push_number(json_t* parent, int value);

void
json_array_push_string(json_t* parent, char* value);

json_t* json_new_comand(int stype, int comand);

void json_object_update_number(json_t* parent,char* key,int value);

void json_object_remove(json_t* json, char* key);

unsigned int json_object_get_unsigned_number(json_t* json, char* key);

int json_object_get_int_number(json_t* json, char* key);

char* json_object_get_string(json_t* json, char* key);


#ifdef __cplusplus
}
#endif

#endif

