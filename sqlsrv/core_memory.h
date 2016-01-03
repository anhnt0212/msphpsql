#ifndef __CORE_MEMORY__
#define __CORE_MEMORY__

#include "core_sqlsrv.h"
#include <zend.h>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include "zend_utility.h"


#if SQLSRV_HOOK_CRT_MALLOC && _DEBUG
#define _CRTDBG_MAP_ALLOC 
#include <crtdbg.h>
#endif

// Centralized location to interact with memory 
// As new tcMalloc/jeMalloc inspired Zend MM is too tight 
// It is to easy to get a heap corruption without even seeing a corruption message unlike previous PHP memory message
// if you even once mix persistent mallocs and nonpersitent frees or vice versa
//
// 1. Every malloc/free/realloc in the project are done here
// 2. Added persistency flags to all allocation functions, STL allocator class and smart pointer classes
// 3. Added debug utility functionality such as programatic memory breakpoints and overriding CRT malloc/frees
//

//
//****** MEMORY LEAK DETECTION ****
//
// WITH ZEND ENGINE :
//
// 1. MS`s original smart ptr types are quite useful 
// 2. If you set ZEND_DEBUG=1 and set report_memleaks=1 in your php.ini file
//    You can see Zend`s messages with Microsoft DbgView , or just output window in VS2015
// 3. ...
//
// WITH MS CRT ( set USE_ZEND_ALLOC = 0 in environment variables to turn the Zend MM off)
//
// 1. During PHP compilation define _CRTDBG_MAP_ALLOC for the preprocessor
// 2. During php compilation inside php7 engine , include crtdbg.h
// 3. In one of PHP SAPI`s entry points call _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF) ( For example : main function in php_cli.c )
// 4. Eiher VS output windows or Dbgview will show leaks with an allocation number
// 5. To place breakpoints to those allocations again in a SAPI entry point place  place _CrtSetBreakAlloc(<allocation_number>)

#if _DEBUG
inline unsigned long insert_data_breakpoint(void* address, DWORD protectionMode = PAGE_EXECUTE)
{
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	DWORD pageSize = sysInfo.dwPageSize;
	DWORD lastProtectValue = 0;
	VirtualProtect(address, pageSize, protectionMode, &lastProtectValue);
	return lastProtectValue;
}
#endif

// DEFINE SQLSRV_MEM_DEBUG=1 TO TURN MEMORY TRACES ON

// https://msdn.microsoft.com/en-us/library/cy8c7wz5.aspx
#if SQLSRV_HOOK_CRT_MALLOC && _DEBUG
inline int crt_malloc_hook(int nAllocType, void *pvData,
  size_t nSize, int nBlockUse, long lRequest,
  const unsigned char * szFileName, int nLine)
{
  if (nBlockUse == _CRT_BLOCK)
  {
    return TRUE;
  }

  switch (nAllocType)
  {
  case _HOOK_ALLOC:
#if SQLSRV_MEM_DEBUG
	printf("CRT malloc %d %d", lRequest, nSize);
#endif
    break;

  case _HOOK_REALLOC:
#if SQLSRV_MEM_DEBUG
	  printf("CRT realloc %d %d", lRequest, nSize);
#endif
    break;

  case _HOOK_FREE:
#if SQLSRV_MEM_DEBUG
	  printf("CRT free %d %d", lRequest, nSize);
#endif
    break;
  }

  return TRUE;
}

inline void hook_crt_malloc()
{
  _CrtSetAllocHook(crt_malloc_hook);
}
#endif

// the macro max is defined and overrides the call to max in the allocator class
#pragma push_macro( "max" )
#undef max

inline void sqlsrv_malloc_resource(zval* zv, int index, void* resource_pointer, int resource_type, bool persistent = false)
{
	if (persistent)
	{
		ZVAL_NEW_PERSISTENT_RES(zv, index, resource_pointer, resource_type);
	}
	else
	{
		ZVAL_NEW_RES(zv, index, resource_pointer, resource_type);
	}
}

inline void mark_hashtable_as_initialised(HashTable* ht)
{
	(ht)->u.flags |= HASH_FLAG_INITIALIZED;
}

inline void make_zval_non_refcounted(zval* zv)
{
	// This one is quite important in order to avoid "compiled variable" freeing
	Z_TYPE_INFO_P(zv) &= ~(IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT);
}

inline void make_zval_refcounted(zval* zv)
{
	// This one is quite important in order to avoid "compiled variable" freeing
	Z_TYPE_INFO_P(zv) |= ~(IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT);
}

inline void* sqlsrv_malloc(size_t size, bool persistent = false)
{
  if (persistent)
  {
    return malloc(size);
  }
  else
  {
    return emalloc(size);
  }
}

inline void* sqlsrv_realloc(void* original_ptr, size_t size, bool persistent = false)
{
  if (persistent)
  {
    return realloc(original_ptr, size);
  }
  else
  {
    return erealloc(original_ptr, size);
  }
}

inline void zval_destructor(zval* val)
{
  zval_ptr_dtor(val);
}

inline void sqlsrv_malloc_zval(zval **ptr, bool persistent = false)
{
  *ptr = (zval *)sqlsrv_malloc(sizeof(zval), persistent);
}

inline void sqlsrv_malloc_hashtable(HashTable **ptr, bool persistent = false)
{
  *ptr = (HashTable *)sqlsrv_malloc(sizeof(HashTable), persistent);
}

inline void sqlsrv_malloc_hashtable(zval **val, bool persistent = false)
{
  sqlsrv_malloc_zval(val);
  sqlsrv_malloc_hashtable(&Z_ARRVAL_P(*val));
}

inline void sqlsrv_new_array(zval* val, bool persistent = false)
{
  if (persistent)
  {
    ZVAL_NEW_PERSISTENT_ARR(val);
  }
  else
  {
    ZVAL_NEW_ARR(val);
  }
}

inline int sqlsrv_new_array_and_init(zval* val, bool persistent = false)
{
  if (persistent == false)
  {
    return array_init(val);
  }
  else
  {
    ZVAL_NEW_PERSISTENT_ARR(val);
    _zend_hash_init(Z_ARRVAL_P(val), 0, ZVAL_PTR_DTOR, 1 ZEND_FILE_LINE_CC);
    return SUCCESS;
  }
}

inline void* sqlsrv_malloc(size_t element_count, size_t element_size, size_t extra, bool persistent = false)
{
  OACR_WARNING_SUPPRESS(ALLOC_SIZE_OVERFLOW_IN_ALLOC_WRAPPER, "Overflow verified below");

  if ((element_count > 0 && element_size > 0) &&
    (element_count > element_size * element_count || element_size > element_size * element_count)) {
    DIE("Integer overflow in sqlsrv_malloc");
  }

  if (element_size * element_count > element_size * element_count + extra) {
    DIE("Integer overflow in sqlsrv_malloc");
  }

  if (element_size * element_count + extra == 0) {
    DIE("Allocation size must be more than 0");
  }

  return sqlsrv_malloc(element_size * element_count + extra, persistent);
}

inline void sqlsrv_free(void* ptr, bool persistent = false)
{
  if (persistent)
  {
    free(ptr);
  }
  else
  {
    efree(ptr);
  }
}

inline void sqlsrv_malloc_zend_string(zval* val, const char* string, size_t len, bool persistent = false)
{
  if (persistent)
  {
    if (len == 0)
      ZVAL_PSTRING(val, string);
    else
      ZVAL_PSTRINGL(val, string, len);
  }
  else
  {
    if (len == 0)
      ZVAL_STRING(val, string);
    else
      ZVAL_STRINGL(val, string, len);
  }
}

inline void sqlsrv_free_zend_string(zend_string* s)
{
	if (s)
	{
		sqlsrv_free(s);
	}
}

/*
enum SQLSRV_PHPTYPE {
	MIN_SQLSRV_PHPTYPE = 1, // lowest value for a php type
	SQLSRV_PHPTYPE_NULL = 1,
	SQLSRV_PHPTYPE_INT,
	SQLSRV_PHPTYPE_FLOAT,
	SQLSRV_PHPTYPE_STRING,
	SQLSRV_PHPTYPE_DATETIME,
	SQLSRV_PHPTYPE_STREAM,
	MAX_SQLSRV_PHPTYPE,      // highest value for a php type
	SQLSRV_PHPTYPE_INVALID = MAX_SQLSRV_PHPTYPE     // used to see if a type is invalid
};
*/
inline void clean_php_variable(SQLSRV_PHPTYPE type, zval* variable)
{
	if (variable == nullptr)
	{
		return;
	}
	switch (type) 
	{
		case SQLSRV_PHPTYPE_STRING:
			{
			sqlsrv_free_zend_string(Z_STR_P(variable));
			break;
			}

		default:
			break;
	}
}

inline void clean_array(zval* val)
{
  zend_array_destroy(Z_ARRVAL_P(val));
}

inline void clean_hashtable(HashTable* ht, bool persistent = false)
{
  zend_hash_destroy((ht));

  if (persistent)
  {
    sqlsrv_free(ht, true);
  }
  else
  {
    FREE_HASHTABLE((ht));
  }
}

inline void null_zval(zval* zv)
{
	ZVAL_NULL(zv);
}

inline void clean_hashtable(zval* val, bool persistent = false)
{
  clean_hashtable((HashTable *)Z_ARRVAL_P(val), persistent);
}

inline void add_ref_zval(zval* val)
{
  zval_add_ref(val);
}

inline void __cdecl zval_with_res_dtor(zval* val)
{
  efree(val->value.res);
}

inline void __cdecl persistent_zval_with_res_dtor(zval* val)
{
  free(val->value.res);
}

////////////////////////////////////////////////////////////////////////////////////
//////////////////// SMART POINTERS
////////////////////////////////////////////////////////////////////////////////////

// trait class that allows us to assign const types to an auto_ptr
template <typename T>
struct remove_const {
	typedef T type;
};

template <typename T>
struct remove_const<const T*> {
	typedef T* type;
};

// base class for auto_ptrs that we define below.  It provides common operators and functions 
// used by all the classes.  
template <typename T, typename Subclass, bool persistency = false>
class sqlsrv_auto_ptr {

public:

	sqlsrv_auto_ptr(void) : _ptr(NULL)
	{
	}

	~sqlsrv_auto_ptr(void)
	{
		static_cast<Subclass*>(this)->reset(NULL);
	}

	// call when ownership is transferred
	T *transferred(void)
	{
		T *rv = _ptr;
		_ptr = NULL;
		return rv;
	}


	// explicit function to get the pointer.
	T* get(void)
	{
		return _ptr;
	}

	// cast operator to allow auto_ptr to be used where a normal const * can be.
	operator T* ()
	{
		return _ptr;
	}

	operator const T* () const
	{
		return (const T*)_ptr;
	}

	// cast operator to allow auto_ptr to be used where a normal pointer can be.
	/*
	WTF.  This casts away const autmatically.  No.
	operator typename remove_const<T*>::type () const
	{
	return _ptr;
	}
	*/

	operator bool() const
	{
		return _ptr != NULL;
	}

	// there are a number of places where we allocate a block intended to be accessed as
	// an array of elements, so this operator allows us to treat the memory as such.
	template <class IX> T& operator[](IX index)
	{
		return _ptr[index];
	}


	// access elements of a structure through the auto ptr
	T* const operator->(void)
	{
		return _ptr;
	}

	// value from reference operator (i.e., i = *(&i); or *i = blah;)
	T& operator*()
	{
		return *_ptr;
	}

	// allow the use of the address-of operator to simulate a **.
	// Note: this operator conflicts with storing these within an STL container.  If you need
	// to do that, then redefine this as getpp and change instances of &auto_ptr to auto_ptr.getpp()
	T** operator&(void)
	{
		return &_ptr;
	}

	bool operator!()
	{
		return _ptr == (T *)NULL;
	}

protected:

	sqlsrv_auto_ptr(T* ptr) : _ptr(ptr)
	{
	}

	sqlsrv_auto_ptr(const sqlsrv_auto_ptr<T, Subclass, persistency> &src) : _ptr(const_cast<sqlsrv_auto_ptr<T, Subclass, persistency> &>(src).transferred())
	{
	}

	sqlsrv_auto_ptr<T, Subclass, persistency>& operator=(const sqlsrv_auto_ptr<T, Subclass, persistency> &ref)
	{
		T *ptr = const_cast<sqlsrv_auto_ptr<T, Subclass, persistency> &>(ref).transferred();
		return operator=(ptr);
	}

	// assign a new pointer to the auto_ptr.  It will free the previous memory block
	// because ownership is deemed finished.
	sqlsrv_auto_ptr<T, Subclass, persistency>& operator=(T* ptr)
	{
		static_cast<Subclass*>(this)->reset(ptr);
		return *this;
	}

	mutable T* _ptr;
};

// an auto_ptr for sqlsrv_malloc/sqlsrv_free.  When allocating a chunk of memory using sqlsrv_malloc, wrap that pointer
// in a variable of sqlsrv_malloc_auto_ptr.  sqlsrv_malloc_auto_ptr will "own" that block and assure that it is
// freed until the variable is destroyed (out of scope) or ownership is transferred using the function
// "transferred".
// DO NOT CALL sqlsrv_realloc with a sqlsrv_malloc_auto_ptr.  Use the resize member function.

template <typename T, bool persistency = false>
class sqlsrv_malloc_auto_ptr : public sqlsrv_auto_ptr<T, sqlsrv_malloc_auto_ptr<T, persistency>, persistency >
{

public:
	sqlsrv_malloc_auto_ptr(T *ptr = (T*)NULL) :
		sqlsrv_auto_ptr<T, sqlsrv_malloc_auto_ptr<T, persistency>, persistency >(ptr)
	{
	}

	sqlsrv_malloc_auto_ptr(const sqlsrv_malloc_auto_ptr<T, persistency> & src) : sqlsrv_auto_ptr<T, sqlsrv_malloc_auto_ptr<T, persistency>, persistency >(src)
	{
	}

	// free the original pointer and assign a new pointer. Use NULL to simply free the pointer.
	void reset(T* ptr = NULL)
	{
		if (_ptr && _ptr != ptr)
		{
			_ptr->~T();
			sqlsrv_free((void*)_ptr, persistency);
		}
		_ptr = ptr;
	}

	sqlsrv_malloc_auto_ptr<T, persistency>& operator=(const sqlsrv_malloc_auto_ptr<T, persistency>& src)
	{
		sqlsrv_auto_ptr<T, sqlsrv_malloc_auto_ptr<T, persistency>, persistency >::operator=(src);
		return *this;
	}

	sqlsrv_malloc_auto_ptr<T, persistency>& operator=(T* ptr)
	{
		sqlsrv_auto_ptr<T, sqlsrv_malloc_auto_ptr<T, persistency>, persistency >::operator=(ptr);
		return *this;
	}

	// DO NOT CALL sqlsrv_realloc with a sqlsrv_malloc_auto_ptr.  Use the resize member function.
	// has the same parameter list as sqlsrv_realloc: new_size is the size in bytes of the newly allocated buffer
	// This only makes sense if _ptr contains pod.
	void resize(size_t new_size)
	{
		_ptr = reinterpret_cast<T*>(sqlsrv_realloc(_ptr, new_size));
	}
};


// auto ptr for Zend hash tables.  Used to clean up a hash table allocated when 
// something caused an early exit from the function.  This is used when the hash_table is
// allocated in a zval that itself can't be released.  Otherwise, use the zval_auto_ptr.

class hash_auto_ptr : public sqlsrv_auto_ptr<HashTable, hash_auto_ptr> {

public:

	hash_auto_ptr(void) :
		sqlsrv_auto_ptr<HashTable, hash_auto_ptr>(NULL)
	{
	}

	// free the original pointer and assign a new pointer. Use NULL to simply free the pointer.
	void reset(HashTable* ptr = NULL)
	{
		if (_ptr && ptr != _ptr)
		{
			clean_hashtable(_ptr);
		}
		_ptr = ptr;
	}

	hash_auto_ptr& operator=(HashTable* ptr)
	{
		sqlsrv_auto_ptr<HashTable, hash_auto_ptr>::operator=(ptr);
		return *this;
	}

	hash_auto_ptr& operator=(const hash_auto_ptr &ref)
	{
		sqlsrv_auto_ptr<HashTable, hash_auto_ptr>::operator=(ref);
		return *this;
	}

private:

	hash_auto_ptr(HashTable const& hash);

	hash_auto_ptr(hash_auto_ptr const& hash);
};


// an auto_ptr for zvals.  When allocating a zval, wrap that pointer in a variable of zval_auto_ptr.  
// zval_auto_ptr will "own" that zval and assure that it is freed when the variable is destroyed 
// (out of scope) or ownership is transferred using the function "transferred".

class zval_auto_ptr : public sqlsrv_auto_ptr<zval, zval_auto_ptr> {

public:

	zval_auto_ptr(void)
	{
	}

	// free the original pointer and assign a new pointer. Use NULL to simply free the pointer.
	void reset(zval* ptr = NULL)
	{
		if (_ptr)
			//PHP7 Port
#if PHP_MAJOR_VERSION >= 7 
			sqlsrv_free(_ptr); //Because unlike php5.5 not all types are re counted ,therefore calling zval dtor will lead to a leak
#else
			zval_ptr_dtor(&_ptr);
#endif
		_ptr = ptr;
	}

	zval_auto_ptr& operator=(zval* ptr)
	{
		sqlsrv_auto_ptr<zval, zval_auto_ptr>::operator=(ptr);
		return *this;
	}
	zval_auto_ptr& operator=(const zval_auto_ptr &ref)
	{
		sqlsrv_auto_ptr<zval, zval_auto_ptr>::operator=(ref);
		return *this;
	}

	//PHP7 Port
#if PHP_MAJOR_VERSION < 7
#if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3)
	operator zval_gc_info*(void)
	{
		return reinterpret_cast<zval_gc_info*>(_ptr);
	}
#endif
#endif

private:

	zval_auto_ptr(const zval_auto_ptr& src);
};

#pragma pop_macro( "max" )

#endif