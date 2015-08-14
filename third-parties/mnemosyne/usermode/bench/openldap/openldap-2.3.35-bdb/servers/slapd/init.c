/* init.c - initialize various things */
/* $OpenLDAP: pkg/ldap/servers/slapd/init.c,v 1.81.2.16 2007/01/25 12:42:38 hyc Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2007 The OpenLDAP Foundation.
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lber_pvt.h"
#ifdef LDAP_SLAPI
#include "slapi/slapi.h"
#endif

#include "ldap_rq.h"

/*
 * read-only global variables or variables only written by the listener
 * thread (after they are initialized) - no need to protect them with a mutex.
 */
int		slap_debug = 0;

#ifdef LDAP_DEBUG
int		ldap_syslog = LDAP_DEBUG_STATS;
#else
int		ldap_syslog;
#endif

#ifdef LOG_DEBUG
int		ldap_syslog_level = LOG_DEBUG;
#endif

BerVarray default_referral = NULL;

struct berval AllUser = BER_BVC( LDAP_ALL_USER_ATTRIBUTES );
struct berval AllOper = BER_BVC( LDAP_ALL_OPERATIONAL_ATTRIBUTES );
struct berval NoAttrs = BER_BVC( LDAP_NO_ATTRS );

/*
 * global variables that need mutex protection
 */
ldap_pvt_thread_pool_t	connection_pool;
int			connection_pool_max = SLAP_MAX_WORKER_THREADS;
int		slap_tool_thread_max = 1;
#ifndef HAVE_GMTIME_R
ldap_pvt_thread_mutex_t	gmtime_mutex;
#endif

slap_counters_t			slap_counters;

/*
 * these mutexes must be used when calling the entry2str()
 * routine since it returns a pointer to static data.
 */
ldap_pvt_thread_mutex_t	entry2str_mutex;
ldap_pvt_thread_mutex_t	replog_mutex;

static const char* slap_name = NULL;
int slapMode = SLAP_UNDEFINED_MODE;

int
slap_init( int mode, const char *name )
{
	int rc;
	int i;

	assert( mode );

	if ( slapMode != SLAP_UNDEFINED_MODE ) {
		/* Make sure we write something to stderr */
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		 "%s init: init called twice (old=%d, new=%d)\n",
		 name, slapMode, mode );

		return 1;
	}

	slapMode = mode;

#ifdef SLAPD_MODULES
	if ( module_init() != 0 ) {
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		    "%s: module_init failed\n",
			name, 0, 0 );
		return 1;
	}
#endif

	if ( slap_schema_init( ) != 0 ) {
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		    "%s: slap_schema_init failed\n",
		    name, 0, 0 );
		return 1;
	}


	switch ( slapMode & SLAP_MODE ) {
	case SLAP_SERVER_MODE:

		/* FALLTHRU */
	case SLAP_TOOL_MODE:
		Debug( LDAP_DEBUG_TRACE,
			"%s init: initiated %s.\n",	name,
			(mode & SLAP_MODE) == SLAP_TOOL_MODE ? "tool" : "server",
			0 );

		slap_name = name;

		ldap_pvt_thread_pool_init( &connection_pool,
				connection_pool_max, 0);
		ldap_pvt_thread_mutex_init( &entry2str_mutex );
		ldap_pvt_thread_mutex_init( &replog_mutex );

		ldap_pvt_thread_mutex_init( &slap_counters.sc_sent_mutex );
		ldap_pvt_thread_mutex_init( &slap_counters.sc_ops_mutex );
		ldap_pvt_mp_init( slap_counters.sc_bytes );
		ldap_pvt_mp_init( slap_counters.sc_pdu );
		ldap_pvt_mp_init( slap_counters.sc_entries );
		ldap_pvt_mp_init( slap_counters.sc_refs );

		ldap_pvt_mp_init( slap_counters.sc_ops_initiated );
		ldap_pvt_mp_init( slap_counters.sc_ops_completed );

		ldap_pvt_thread_mutex_init( &slapd_rq.rq_mutex );
		LDAP_STAILQ_INIT( &slapd_rq.task_list );
		LDAP_STAILQ_INIT( &slapd_rq.run_list );

#ifdef SLAPD_MONITOR
		for ( i = 0; i < SLAP_OP_LAST; i++ ) {
			ldap_pvt_mp_init( slap_counters.sc_ops_initiated_[ i ] );
			ldap_pvt_mp_init( slap_counters.sc_ops_completed_[ i ] );
		}
#endif /* SLAPD_MONITOR */

#ifndef HAVE_GMTIME_R
		ldap_pvt_thread_mutex_init( &gmtime_mutex );
#endif
		slap_passwd_init();

		rc = slap_sasl_init();

		if( rc == 0 ) {
			rc = backend_init( );
		}
		if ( rc )
			return rc;

		break;

	default:
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
			"%s init: undefined mode (%d).\n", name, mode, 0 );

		rc = 1;
		break;
	}

	if ( slap_controls_init( ) != 0 ) {
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		    "%s: slap_controls_init failed\n",
		    name, 0, 0 );
		return 1;
	}

#ifdef HAVE_TLS
	/* Library defaults to full certificate checking. This is correct when
	 * a client is verifying a server because all servers should have a
	 * valid cert. But few clients have valid certs, so we want our default
	 * to be no checking. The config file can override this as usual.
	 */
	rc = 0;
	(void) ldap_pvt_tls_set_option( NULL, LDAP_OPT_X_TLS_REQUIRE_CERT, &rc );
#endif

	if ( frontend_init() ) {
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		    "%s: frontend_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	if ( overlay_init() ) {
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		    "%s: overlay_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	if ( glue_sub_init() ) {
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		    "%s: glue/subordinate init failed\n",
		    name, 0, 0 );

		return 1;
	}

	if ( acl_init() ) {
		ldap_debug |= 1;
		Debug( LDAP_DEBUG_ANY,
		    "%s: acl_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	return rc;
}

int slap_startup( Backend *be )
{
	int rc;

	Debug( LDAP_DEBUG_TRACE,
		"%s startup: initiated.\n",
		slap_name, 0, 0 );


	rc = backend_startup( be );

#ifdef LDAP_SLAPI
	if( rc == 0 ) {
		Slapi_PBlock *pb = slapi_pblock_new();

		if ( slapi_int_call_plugins( frontendDB, SLAPI_PLUGIN_START_FN, pb ) < 0 ) {
			rc = -1;
		}
		slapi_pblock_destroy( pb );
	}
#endif /* LDAP_SLAPI */

	return rc;
}

int slap_shutdown( Backend *be )
{
	int rc;
#ifdef LDAP_SLAPI
	Slapi_PBlock *pb;
#endif

	Debug( LDAP_DEBUG_TRACE,
		"%s shutdown: initiated\n",
		slap_name, 0, 0 );

	/* let backends do whatever cleanup they need to do */
	rc = backend_shutdown( be ); 

#ifdef LDAP_SLAPI
	pb = slapi_pblock_new();
	(void) slapi_int_call_plugins( frontendDB, SLAPI_PLUGIN_CLOSE_FN, pb );
	slapi_pblock_destroy( pb );
#endif /* LDAP_SLAPI */

	return rc;
}

int slap_destroy(void)
{
	int rc;
	int i;

	Debug( LDAP_DEBUG_TRACE,
		"%s destroy: freeing system resources.\n",
		slap_name, 0, 0 );

	if ( default_referral ) {
		ber_bvarray_free( default_referral );
	}

	/* clear out any thread-keys for the main thread */
	ldap_pvt_thread_pool_context_reset( ldap_pvt_thread_pool_context());

	rc = backend_destroy();

	slap_sasl_destroy();

	entry_destroy();

	switch ( slapMode & SLAP_MODE ) {
	case SLAP_SERVER_MODE:
	case SLAP_TOOL_MODE:

		ldap_pvt_thread_mutex_destroy( &slap_counters.sc_sent_mutex );
		ldap_pvt_thread_mutex_destroy( &slap_counters.sc_ops_mutex );
		ldap_pvt_mp_clear( slap_counters.sc_bytes );
		ldap_pvt_mp_clear( slap_counters.sc_pdu );
		ldap_pvt_mp_clear( slap_counters.sc_entries );
		ldap_pvt_mp_clear( slap_counters.sc_refs );
		ldap_pvt_mp_clear( slap_counters.sc_ops_initiated );
		ldap_pvt_mp_clear( slap_counters.sc_ops_completed );

#ifdef SLAPD_MONITOR
		for ( i = 0; i < SLAP_OP_LAST; i++ ) {
			ldap_pvt_mp_clear( slap_counters.sc_ops_initiated_[ i ] );
			ldap_pvt_mp_clear( slap_counters.sc_ops_completed_[ i ] );
		}
#endif /* SLAPD_MONITOR */
		break;

	default:
		Debug( LDAP_DEBUG_ANY,
			"slap_destroy(): undefined mode (%d).\n", slapMode, 0, 0 );

		rc = 1;
		break;

	}

	ldap_pvt_thread_destroy();

	/* should destory the above mutex */
	return rc;
}
