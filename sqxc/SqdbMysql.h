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

#ifndef SQDB_MYSQL_H
#define SQDB_MYSQL_H

#include <mysql/mysql.h>

#include <Sqdb.h>

// ----------------------------------------------------------------------------
// C/C++ common declarations: declare type, structue, macro, enumeration.

typedef struct SqdbMysql           SqdbMysql;
typedef struct SqdbConfigMysql     SqdbConfigMysql;

// ----------------------------------------------------------------------------
// C declarations: declare C data, function, and others.

#ifdef __cplusplus
extern "C" {
#endif

extern const SqdbInfo    *SQDB_INFO_MYSQL;

#define sqdb_mysql_new(sqdb_config)    sqdb_new(SQDB_INFO_MYSQL, sqdb_config)

#ifdef __cplusplus
}  // extern "C"
#endif

// ----------------------------------------------------------------------------
// C/C++ common definitions: define structue

/*
    SqdbMysql - Sqdb for MySQL

    Sqdb
    |
    `--- SqdbMysql

    The correct way to derive Sqdb:  (conforming C++11 standard-layout)
    1. Use Sq::DbMethod to inherit member function(method).
    2. Use SQDB_MEMBERS to inherit member variable.
    3. Add variable and non-virtual function in derived struct.
    ** This can keep std::is_standard_layout<>::value == true
 */

#ifdef __cplusplus
struct SqdbMysql : Sq::DbMethod            // <-- 1. inherit C++ member function(method)
#else
struct SqdbMysql
#endif
{
	SQDB_MEMBERS;                          // <-- 2. inherit member variable
/*	// ------ Sqdb members ------
	const SqdbInfo *info;

	// schema version in SQL database
	int             version;
 */

	// ------ SqdbMysql members ------     // <-- 3. Add variable and non-virtual function in derived struct.
	MYSQL *self;
	SqdbConfigMysql *config;
};

/*
    SqdbConfigMysql - SqdbMysql use this configure

    SqdbConfig
    |
    `--- SqdbConfigMysql

    Note: use 'const char*' to declare string here, C++ user can initialize static struct easily.
 */
struct SqdbConfigMysql
{
	SQDB_CONFIG_MEMBERS;
/*	// ------ SqdbConfig members ------
	unsigned int    product;
	unsigned int    bit_field;
 */

	// ------ SqdbConfigMysql members ------
	const char   *host;
	const char   *user;
	const char   *password;
	const char   *db;
	unsigned int  port;
};


// ----------------------------------------------------------------------------
// C++ definitions: define C++ data, function, method, and others.

#ifdef __cplusplus

namespace Sq {

// --- C++11 standard-layout ---

typedef struct SqdbConfigMysql    DbConfigMysql;

// conforming C++11 standard-layout
// These are for directly use only. You can NOT derived it.
struct DbMysql : SqdbMysql
{
	DbMysql(SqdbConfigMysql *config = NULL) {
		this->info = SQDB_INFO_MYSQL;  SQDB_INFO_MYSQL->init((Sqdb*)this, (SqdbConfig*)config);
	}
	~DbMysql() {
		SQDB_INFO_MYSQL->final((Sqdb*)this);
	}
};

};  // namespace Sq

#endif  // __cplusplus


#endif  // SQDB_MYSQL_H
