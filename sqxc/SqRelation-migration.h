/*
 *   Copyright (C) 2020-2021 by C.H. Huang
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

#ifndef SQ_RELATION_MIGRATION_H
#define SQ_RELATION_MIGRATION_H

#include <SqType.h>
#include <SqRelation.h>
#include <SqSchema.h>

// ----------------------------------------------------------------------------
// C/C++ common declarations: declare type, structue, macro, enumeration.

// These types are for internal use only
#define SQ_TYPE_FOREIGN    SQ_TYPE_FAKE2
#define SQ_TYPE_COMPOSITE  SQ_TYPE_FAKE3
#define SQ_TYPE_RESERVE    SQ_TYPE_FAKE4
#define SQ_TYPE_REENTRY    SQ_TYPE_FAKE5

// ----------------------------------------------------------------------------
// C declarations: declare C data, function, and others.

#ifdef __cplusplus
extern "C" {
#endif

/* --- SqRelation functions --- */

// This function trace related list of SQ_TYPE_REENTRY by SqReentry.old_name.
// if 'erase_traced' == true, it will erase node that has been traced in related list of SQ_TYPE_REENTRY.
const char *sq_relation_trace_reentry(SqRelation *relation, const char *old_name, bool erase_traced);

/* --- SqTable functions --- */

void  sq_table_create_relation(SqTable *table, SqRelationPool *pool);

/* --- SqSchema functions --- */

void  sq_schema_create_relation(SqSchema *schema);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SQ_RELATION_MIGRATION_H
