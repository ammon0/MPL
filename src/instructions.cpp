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


#include <mpl/instructions.hpp>
#include <util/data.h>


/******************************************************************************/
//                           BASIC BLOCK FUNCTIONS
/******************************************************************************/


        Block:: Block (void     )     { q = DS_new_list(sizeof(Instruction)); }
        Block::~Block (void     )     {                 DS_delete  (q   )   ; }
uint    Block::count  (void     )const{ return          DS_count   (q   )   ; }
void    Block::flush  (void     )     {                 DS_flush   (q   )   ; }
inst_pt Block::next   (void     )const{ return (inst_pt)DS_next    (q   )   ; }
inst_pt Block::prev   (void     )const{ return (inst_pt)DS_previous(q   )   ; }
inst_pt Block::first  (void     )const{ return (inst_pt)DS_first   (q   )   ; }
inst_pt Block::last   (void     )const{ return (inst_pt)DS_last    (q   )   ; }
inst_pt Block::enqueue(inst_pt i)     { return (inst_pt)DS_nq      (q, i)   ; }
inst_pt Block::remove (void     )     { return (inst_pt)DS_remove  (q   )   ; }



