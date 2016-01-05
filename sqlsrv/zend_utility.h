#ifndef __ZEND_UTILITY__
#define __ZEND_UTILITY__

// This header file contains macros that existed in prePHP7 
// but now don`t exist in PHP7 to enable quickly porting PHP extensions to PHP7
#include <zend.h>
#include <zend_globals.h>
#include <php_version.h>

#include <php.h>
// DONT INCLUDE IN RELEASE MODE AS IT IS TAKING AGES WITH THIS HEADER
// ALSO CHANGED A FEW IOSTREAM INCLUSIONS IN ORIGINAL SQLSRV CPP FILES TO IOSFWD
#ifdef _DEBUG    
#include <iostream>
#endif
#include <cstddef>

// Kill the PHP process and log the message to PHP
void die(const char* msg, ...);
#define DIE( msg, ... ) { die( msg, __VA_ARGS__ ); }

// OACR is an internal Microsoft static code analysis tool
#if defined(OACR)
#include <oacr.h>
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(ALLOC_SIZE_OVERFLOW, "Third party code.")
OACR_WARNING_DISABLE(INDEX_NEGATIVE, "Third party code.")
OACR_WARNING_DISABLE(UNANNOTATED_BUFFER, "Third party code.")
OACR_WARNING_DISABLE(INDEX_UNDERFLOW, "Third party code.")
OACR_WARNING_DISABLE(REALLOCLEAK, "Third party code.")
OACR_WARNING_DISABLE(ALLOC_SIZE_OVERFLOW_WITH_ACCESS, "Third party code.")
#else
// define to eliminate static analysis hints in the code
#define OACR_WARNING_SUPPRESS( warning, msg )
#endif

#if PHP_MAJOR_VERSION >= 7

namespace // Following MS`s anonymous namespace usage, not to pollute the global namespace
{
	const int MAX_ERROR_MESSAGE_LEN = 128;
}

// Find a zend resource in the global resource list of Zend engine
// and return it by zval
inline zend_resource* zend_list_find(HashTable* resources, long res_handle, int type)
{
	zend_resource* ret = NULL;

	auto temp = resources->arData[res_handle].val.value.res;

	if( temp )
	{
		if (temp->type == type)
		{
			ret = temp;
		}
	}

	return ret;
}

inline std::size_t get_zend_string_len(const zend_string* const str)
{
	//We add 1 because zend_string len`s don`t count null termination
	//In most places functions expect null-terminated lengths
	return str->len + 1;
}

inline void non_supported_function(const char* message)
{
	char errorMessage[MAX_ERROR_MESSAGE_LEN] = { (char)NULL };
	snprintf(errorMessage, MAX_ERROR_MESSAGE_LEN, "%s currently not supported", message);
	php_error(E_ERROR, errorMessage);
}

#ifdef _DEBUG 
inline void dump_zval(zval* val)
{
	auto typeZval = Z_TYPE_P(val);

	switch (typeZval)
	{
		case IS_NULL:
			std::cout << "Zval type is null" << std::endl;
			break;

		case IS_FALSE:
			std::cout << "Zval type is false" << std::endl;
			break;

		case IS_TRUE:
			std::cout << "Zval type is true" << std::endl;
			break;

		case IS_LONG:
			std::cout << "Zval type is long : " << val->value.lval << std::endl;
			break;

		case IS_DOUBLE:
			std::cout << "Zval type is double : " << val->value.dval << std::endl;
			break;

		case IS_STRING:
			std::cout << "Zval type is string : " << val->value.str->val << std::endl;
			break;

		case IS_ARRAY:
			std::cout << "Zval type is array" << std::endl;
			break;

		case IS_OBJECT:
			std::cout << "Zval type is object" << std::endl;
			break;

		case IS_RESOURCE:
			std::cout << "Zval type is resource" << std::endl;
			break;

		case IS_REFERENCE: // IT WASN`T A TYPE BEFORE PHP7
			std::cout << "Zval type is reference" << std::endl;
			break;

		default:
			std::cout << "Unexpected zval type : " << typeZval << std::endl;
			return;
	}

	std::cout << "Zval value : ";
	zend_print_variable(val);
}

inline int dump_zend_array(HashTable* arr)
{
	int i = 0;
	zend_hash_internal_pointer_reset(arr);
	auto test = zend_hash_has_more_elements(arr);

	for (zend_hash_internal_pointer_reset(arr);
	zend_hash_has_more_elements(arr) == SUCCESS;
		zend_hash_move_forward(arr))
	{
		zval* element = NULL;
		if ((element = ::zend_hash_get_current_data(arr)) == NULL)
		{
			std::cout << "Error : dump_zend_array , zend_hash_get_current_data returned null" << std::endl;
		}

		dump_zval(element);
		i++;
	}

	std::cout << std::endl << "Its internal pointer is " << arr->nInternalPointer << " on " << &(arr->nInternalPointer) << std::endl;
	return i;
}

inline void dump_zend_array(const zval* val)
{
	HashTable* arr = Z_ARRVAL_P(val);
	auto n = dump_zend_array(arr);
	std::cout << std::endl << "Array " << val << " has " << n << " elements" << std::endl;
}

inline void dump_zend_array_internal_pointer(const zval* val)
{
	auto intPtrAddr = &((HashTable *)(val))->nInternalPointer;
	std::cout << std::endl << "Its internal pointer is " << *intPtrAddr << " on " << intPtrAddr << std::endl;
}

inline void dump_zend_engine_memory_usage()
{
	// zend_memory_usage , look at heap struct
	std::cout << "Zend size of all allocated pages : " << zend_memory_usage(1) << std::endl;
	std::cout << "Zend size of used allocations : " << zend_memory_usage(0) << std::endl;
}
	
#endif // _DEBUG

#endif // #if PHP_MAJOR_VERSION >= 7
#endif // #ifndef __ZEND_EX__