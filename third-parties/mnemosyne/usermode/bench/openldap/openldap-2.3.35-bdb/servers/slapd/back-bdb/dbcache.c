/* dbcache.c - manage cache of open databases */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-bdb/dbcache.c,v 1.38.2.5 2007/01/02 21:44:00 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2007 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <sys/stat.h>

#include "slap.h"
#include "back-bdb.h"
#include "lutil_hash.h"

#ifdef BDB_INDEX_USE_HASH
/* Pass-thru hash function. Since the indexer is already giving us hash
 * values as keys, we don't need BDB to re-hash them.
 */
static u_int32_t
bdb_db_hash(
	DB *db,
	const void *bytes,
	u_int32_t length
)
{
	u_int32_t ret = 0;
	unsigned char *dst = (unsigned char *)&ret;
	const unsigned char *src = (const unsigned char *)bytes;

	if ( length > sizeof(u_int32_t) )
		length = sizeof(u_int32_t);

	while ( length ) {
		*dst++ = *src++;
		length--;
	}
	return ret;
}
#define	BDB_INDEXTYPE	DB_HASH
#else
#define	BDB_INDEXTYPE	DB_BTREE
#endif

int
bdb_db_cache(
	Backend	*be,
	const char *name,
	DB **dbout )
{
	int i, flags;
	int rc;
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	struct bdb_db_info *db;
	char *file;

	*dbout = NULL;

	for( i=BDB_NDB; i < bdb->bi_ndatabases; i++ ) {
		if( !strcmp( bdb->bi_databases[i]->bdi_name, name) ) {
			*dbout = bdb->bi_databases[i]->bdi_db;
			return 0;
		}
	}

	ldap_pvt_thread_mutex_lock( &bdb->bi_database_mutex );

	/* check again! may have been added by another thread */
	for( i=BDB_NDB; i < bdb->bi_ndatabases; i++ ) {
		if( !strcmp( bdb->bi_databases[i]->bdi_name, name) ) {
			*dbout = bdb->bi_databases[i]->bdi_db;
			ldap_pvt_thread_mutex_unlock( &bdb->bi_database_mutex );
			return 0;
		}
	}

	if( i >= BDB_INDICES ) {
		ldap_pvt_thread_mutex_unlock( &bdb->bi_database_mutex );
		return -1;
	}

	db = (struct bdb_db_info *) ch_calloc(1, sizeof(struct bdb_db_info));

	db->bdi_name = ch_strdup( name );

	rc = db_create( &db->bdi_db, bdb->bi_dbenv, 0 );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_cache: db_create(%s) failed: %s (%d)\n",
			bdb->bi_dbenv_home, db_strerror(rc), rc );
		ldap_pvt_thread_mutex_unlock( &bdb->bi_database_mutex );
		return rc;
	}

	rc = db->bdi_db->set_pagesize( db->bdi_db, BDB_PAGESIZE );
#ifdef BDB_INDEX_USE_HASH
	rc = db->bdi_db->set_h_hash( db->bdi_db, bdb_db_hash );
#endif
	rc = db->bdi_db->set_flags( db->bdi_db, DB_DUP | DB_DUPSORT );

	file = ch_malloc( strlen( name ) + sizeof(BDB_SUFFIX) );
	sprintf( file, "%s" BDB_SUFFIX, name );

#ifdef HAVE_EBCDIC
	__atoe( file );
#endif
	flags = DB_CREATE | DB_THREAD;
#ifdef DB_AUTO_COMMIT
	if ( !( slapMode & SLAP_TOOL_QUICK ))
		flags |= DB_AUTO_COMMIT;
#endif
	/* Cannot Truncate when Transactions are in use */
	if ( (slapMode & (SLAP_TOOL_QUICK|SLAP_TRUNCATE_MODE)) ==
		(SLAP_TOOL_QUICK|SLAP_TRUNCATE_MODE))
			flags |= DB_TRUNCATE;

	rc = DB_OPEN( db->bdi_db,
		file, NULL /* name */,
		BDB_INDEXTYPE, bdb->bi_db_opflags | flags, bdb->bi_dbenv_mode );

	ch_free( file );

	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_cache: db_open(%s) failed: %s (%d)\n",
			name, db_strerror(rc), rc );
		ldap_pvt_thread_mutex_unlock( &bdb->bi_database_mutex );
		return rc;
	}

	bdb->bi_databases[i] = db;
	bdb->bi_ndatabases = i+1;

	*dbout = db->bdi_db;

	ldap_pvt_thread_mutex_unlock( &bdb->bi_database_mutex );
	return 0;
}
