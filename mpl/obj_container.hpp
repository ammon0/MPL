/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the license terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/MPL
 *
 ******************************************************************************/


#ifndef _OBJ_CONTAINER_HPP
#define _OBJ_CONTAINER_HPP


#include <mpl/def.hpp>

typedef struct _root* DS;


class Obj_Index{
	DS index;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Obj_Index(void);
	~Obj_Index(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool  isempty(void);
	
	Object * find (const char * name) const; ///< find an object by name
	Object * first(      void       ) const; ///< Returns the first object
	Object * next (      void       ) const; ///< Returns the next object
	
	/******************************* MUTATORS *********************************/
	
	Object * remove(const char   * name  ); ///< Remove an object by its name
	Object * add   (      Object * object); ///< add a new object
	
};


//class Obj_List{
//	
//	
//public:
//	/****************************** CONSTRUCTOR *******************************/
//	
//	Obj_List(void);
//	
//	/******************************* ACCESSOR *********************************/
//	
//	
//	
//	/******************************* MUTATORS *********************************/
//	
//	
//	
//};

#endif // _OBJ_CONTAINER_HPP


