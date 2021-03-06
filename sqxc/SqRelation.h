/*
 *   Copyright (C) 2021 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 * sqxc is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef SQ_RELATION_H
#define SQ_RELATION_H

#include <SqPtrArray.h>

// ----------------------------------------------------------------------------
// C/C++ common declarations: declare type, structue, macro, enumeration.

typedef struct SqRelation        SqRelation;
typedef struct SqRelationNode    SqRelationNode;
typedef struct SqRelationPool    SqRelationPool;

// ----------------------------------------------------------------------------
// C declarations: declare C data, function, and others.

#ifdef __cplusplus
extern "C" {
#endif

/* --- SqRelation functions --- */

SqRelation *sq_relation_init(SqRelation *relation, SqRelationPool *pool, int capacity);
SqRelation *sq_relation_final(SqRelation *relation);

// SqRelation *sq_relation_new(SqRelationPool *rpool, int capacity);
#define sq_relation_new(pool, capacity)    sq_relation_init((SqRelation*)malloc(sizeof(SqRelation)), pool, capacity)
// void        sq_relation_free(SqRelation *relation);
#define sq_relation_free(relation)         free(sq_relation_final(relation))

void  sq_relation_clear(SqRelation *relation);

// *** function parameter 'no_reverse' set to 1 if user don't need reverse reference

// This function prepend 'to_object' to related list of 'from_object'
// it does NOT check duplicate 'to_object' in related list of 'from_object'
// if 'to_object' is NULL, it will add 'from_object' without reference to 'to_object'
// if 'no_reverse' ==  1, don't add reverse reference
// if 'no_reverse' ==  0, add reverse reference ('to_object' reference to 'from_object')
void  sq_relation_add(SqRelation *relation, const void *from_object, const void *to_object, int no_reverse);

// if 'to_object' is NULL, it will erase all relation that referenced by 'from_object'
// if 'no_reverse' ==  1, don't touch reverse reference
// if 'no_reverse' ==  0, erase reverse reference
// if 'no_reverse' == -1, erase all references in reverse related objects. 'to_object_free_func' must use with this mode.
void  sq_relation_erase(SqRelation *relation, const void *from_object, const void *to_object, int no_reverse, SqDestroyFunc to_object_free_func);

// it replace 'old_object' to 'new_object'.
// if 'no_reverse' ==  1, don't replace 'old_object' in reverse reference
// if 'no_reverse' ==  0, replace 'old_object' in reverse reference
void  sq_relation_replace(SqRelation *relation, const void *old_object, const void *new_object, int no_reverse);

// remove object that doesn't reference to any object.
void  sq_relation_remove_empty(SqRelation *relation);

// return NULL if relation not found. User can NOT free returned value.
// if 'to_object' is NULL, it return SqRelationNode of 'from_object' object.
SqRelationNode *sq_relation_find(SqRelation *relation, const void *from_object, const void *to_object);

/* --- SqRelationNode functions --- */

SqRelationNode *sq_relation_node_find(SqRelationNode *node, const void *object, SqRelationNode **prev_of_returned_node);
SqRelationNode *sq_relation_node_reverse(SqRelationNode *node);

/* --- SqRelationPool functions --- */

SqRelationPool *sq_relation_pool_create(unsigned int chunk_size);
void            sq_relation_pool_destroy(SqRelationPool *pool);
SqRelationNode *sq_relation_pool_alloc(SqRelationPool *pool);
void            sq_relation_pool_free(SqRelationPool *pool, SqRelationNode *node);

#ifdef __cplusplus
}  // extern "C"
#endif

// ----------------------------------------------------------------------------
// C++ declarations: declare C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

/* --- declare methods for Sq::Relation --- */
struct RelationMethod {
	void  add(const void *from_object, const void *to_object, int no_reverse = 0);
	void  erase(const void *from_object, const void *to_object, int no_reverse = 0, SqDestroyFunc to_object_free_func = NULL);
	void  replace(const void *old_object, const void *new_object, int no_reverse = 0);
	void  removeEmpty();
	SqRelationNode *find(const void *from_object, const void *to_object);
};

/* --- declare methods for Sq::RelationNode --- */
struct RelationNodeMethod {
	SqRelationNode *find(const void *object, SqRelationNode **prev_of_returned_node = NULL);
	SqRelationNode *reverse();
};

}  // namespace Sq

#endif  // __cplusplus

// ----------------------------------------------------------------------------
// C/C++ common definitions: define structue

/*
    SqRelation: record relation of object
                SqRelationNode is the element type in in pool and array
 */
#ifdef __cplusplus
struct SqRelation : Sq::RelationMethod {
#else
struct SqRelation {
#endif
	SQ_PTR_ARRAY_MEMBERS(SqRelationNode, data, x2length);    // sorted by object address.
	SqRelationPool  *pool;

#ifdef __cplusplus
	SqRelation(SqRelationPool *pool, int capacity) {
		sq_relation_init((SqRelation*)this, pool, capacity);
	}
	~SqRelation() {
		sq_relation_final((SqRelation*)this);
	}
#endif
};

/*
    SqRelationNode: relation node. This is element type in chunk and array
                    size of this structure == size of 2 pointers because SqRelationNode has 2 pointers
 */
#ifdef __cplusplus
struct SqRelationNode : Sq::RelationNodeMethod {
#else
struct SqRelationNode {
#endif
	void           *object;
	SqRelationNode *next;
};

// ----------------------------------------------------------------------------
// C/C++ common definitions: define global inline function

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) || defined(__cplusplus)
// define inline functions here if compiler supports inline function.

#else   // __STDC_VERSION__ || __cplusplus
// declare functions here if compiler does NOT support inline function.

#endif  // __STDC_VERSION__ || __cplusplus

// ----------------------------------------------------------------------------
// C++ definitions: define C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

/* --- define methods for Sq::Relation --- */
inline void  RelationMethod::add(const void *from_object, const void *to_object, int no_reverse) {
    sq_relation_add((SqRelation*)this, from_object, to_object, no_reverse);
}
inline void  RelationMethod::erase(const void *from_object, const void *to_object, int no_reverse, SqDestroyFunc to_object_free_func) {
    sq_relation_erase((SqRelation*)this, from_object, to_object, no_reverse, to_object_free_func);
}
inline void  RelationMethod::replace(const void *old_object, const void *new_object, int no_reverse) {
    sq_relation_replace((SqRelation*)this, old_object, new_object, no_reverse);
}
inline void  RelationMethod::removeEmpty() {
    sq_relation_remove_empty((SqRelation*)this);
}
inline SqRelationNode *RelationMethod::find(const void *from_object, const void *to_object) {
    return sq_relation_find((SqRelation*)this, from_object, to_object);
}

/* --- define methods for Sq::RelationNode --- */
inline SqRelationNode *RelationNodeMethod::find(const void *object, SqRelationNode **prev) {
    return sq_relation_node_find((SqRelationNode*)this, object, prev);
}

/* --- define C++11 standard-layout structures --- */
typedef struct SqRelation        Relation;
typedef struct SqRelationNode    RelationNode;
typedef struct SqRelationPool    RelationPool;

};  // namespace Sq

#endif  // __cplusplus


#endif  // SQ_RELATION_H
