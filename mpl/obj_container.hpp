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


#include <mpl/object.hpp>

typedef struct _root* DS;


class Obj_Index{
	DS index;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Obj_Index(void);
	~Obj_Index(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool  isempty(void);
	
	obj_pt find (const char * name) const; ///< find an object by name
	obj_pt first(      void       ) const; ///< Returns the first object
	obj_pt next (      void       ) const; ///< Returns the next object
	
	/******************************* MUTATORS *********************************/
	
	obj_pt remove(const char * name  ); ///< Remove an object by its name
	obj_pt add   (      obj_pt object); ///< add a new object
	
};


class Obj_List{
	DS list;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Obj_List(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool isempty(void);
	umax count  (void);
	
	obj_pt find   (const char * name) const;
	obj_pt first  (void             ) const;
	obj_pt next   (void             ) const;
	
	/******************************* MUTATORS *********************************/
	
	obj_pt remove(const char * name  ); ///< Remove an object by its name
	obj_pt add   (      obj_pt object); ///< add a new object
	
};

#endif // _OBJ_CONTAINER_HPP


