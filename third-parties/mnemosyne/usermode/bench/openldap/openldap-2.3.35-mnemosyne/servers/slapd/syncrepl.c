/* syncrepl.c -- Replication Engine which uses the LDAP Sync protocol */
/* $OpenLDAP: pkg/ldap/servers/slapd/syncrepl.c,v 1.168.2.47 2007/04/06 21:49:16 hyc Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2007 The OpenLDAP Foundation.
 * Portions Copyright 2003 by IBM Corporation.
 * Portions Copyright 2003 by Howard Chu, Symas Corporation.
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

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "lutil_ldap.h"

#include "config.h"

#include "ldap_rq.h"

struct nonpresent_entry {
	struct berval *npe_name;
	struct berval *npe_nname;
	LDAP_LIST_ENTRY(nonpresent_entry) npe_link;
};

#define	SYNCDATA_DEFAULT	0	/* entries are plain LDAP entries */
#define	SYNCDATA_ACCESSLOG	1	/* entries are accesslog format */
#define	SYNCDATA_CHANGELOG	2	/* entries are changelog format */

#define	SYNCLOG_LOGGING		0	/* doing a log-based update */
#define	SYNCLOG_FALLBACK	1	/* doing a full refresh */

#define RETRYNUM_FOREVER	(-1)	/* retry forever */
#define RETRYNUM_TAIL		(-2)	/* end of retrynum array */
#define RETRYNUM_VALID(n)	((n) >= RETRYNUM_FOREVER)	/* valid retrynum */
#define RETRYNUM_FINITE(n)	((n) > RETRYNUM_FOREVER)	/* not forever */

typedef struct syncinfo_s {
	struct slap_backend_db *si_be;
	struct re_s			*si_re;
	long				si_rid;
	slap_bindconf		si_bindconf;
	struct berval		si_base;
	struct berval		si_logbase;
	struct berval		si_filterstr;
	struct berval		si_logfilterstr;
	int					si_scope;
	int					si_attrsonly;
	char				*si_anfile;
	AttributeName		*si_anlist;
	AttributeName		*si_exanlist;
	char 				**si_attrs;
	char				**si_exattrs;
	int					si_allattrs;
	int					si_allopattrs;
	int					si_schemachecking;
	int					si_type;	/* the active type */
	int					si_ctype;	/* the configured type */
	time_t				si_interval;
	time_t				*si_retryinterval;
	int					*si_retrynum_init;
	int					*si_retrynum;
	struct sync_cookie	si_syncCookie;
	int					si_manageDSAit;
	int					si_slimit;
	int					si_tlimit;
	int					si_refreshDelete;
	int					si_refreshPresent;
	int					si_syncdata;
	int					si_logstate;
	int					si_conn_setup;
	Avlnode				*si_presentlist;
	LDAP				*si_ld;
	LDAP_LIST_HEAD(np, nonpresent_entry) si_nonpresentlist;
	ldap_pvt_thread_mutex_t	si_mutex;
} syncinfo_t;

static int syncuuid_cmp( const void *, const void * );
static void avl_ber_bvfree( void * );
static void syncrepl_del_nonpresent( Operation *, syncinfo_t *, BerVarray, struct berval * );
static int syncrepl_message_to_op(
					syncinfo_t *, Operation *, LDAPMessage * );
static int syncrepl_message_to_entry(
					syncinfo_t *, Operation *, LDAPMessage *,
					Modifications **, Entry **, int );
static int syncrepl_entry(
					syncinfo_t *, Operation*, Entry*,
					Modifications**,int, struct berval*,
					struct sync_cookie *,
					struct berval * );
static void syncrepl_updateCookie(
					syncinfo_t *, Operation *, struct berval *,
					struct sync_cookie * );
static struct berval * slap_uuidstr_from_normalized(
					struct berval *, struct berval *, void * );

/* callback functions */
static int dn_callback( struct slap_op *, struct slap_rep * );
static int nonpresent_callback( struct slap_op *, struct slap_rep * );
static int null_callback( struct slap_op *, struct slap_rep * );

static AttributeDescription *sync_descs[4];

static void
init_syncrepl(syncinfo_t *si)
{
	int i, j, k, l, n;
	char **attrs, **exattrs;

	if ( !sync_descs[0] ) {
		sync_descs[0] = slap_schema.si_ad_objectClass;
		sync_descs[1] = slap_schema.si_ad_structuralObjectClass;
		sync_descs[2] = slap_schema.si_ad_entryCSN;
		sync_descs[3] = NULL;
	}

	if ( si->si_allattrs && si->si_allopattrs )
		attrs = NULL;
	else
		attrs = anlist2attrs( si->si_anlist );

	if ( attrs ) {
		if ( si->si_allattrs ) {
			i = 0;
			while ( attrs[i] ) {
				if ( !is_at_operational( at_find( attrs[i] ))) {
					for ( j = i; attrs[j] != NULL; j++ ) {
						if ( j == i )
							ch_free( attrs[i] );
						attrs[j] = attrs[j+1];
					}
				} else {
					i++;
				}
			}
			attrs = ( char ** ) ch_realloc( attrs, (i + 2)*sizeof( char * ) );
			attrs[i] = ch_strdup("*");
			attrs[i + 1] = NULL;

		} else if ( si->si_allopattrs ) {
			i = 0;
			while ( attrs[i] ) {
				if ( is_at_operational( at_find( attrs[i] ))) {
					for ( j = i; attrs[j] != NULL; j++ ) {
						if ( j == i )
							ch_free( attrs[i] );
						attrs[j] = attrs[j+1];
					}
				} else {
					i++;
				}
			}
			attrs = ( char ** ) ch_realloc( attrs, (i + 2)*sizeof( char * ) );
			attrs[i] = ch_strdup("+");
			attrs[i + 1] = NULL;
		}

		for ( i = 0; sync_descs[i] != NULL; i++ ) {
			j = 0;
			while ( attrs[j] ) {
				if ( !strcmp( attrs[j], sync_descs[i]->ad_cname.bv_val )) {
					for ( k = j; attrs[k] != NULL; k++ ) {
						if ( k == j )
							ch_free( attrs[k] );
						attrs[k] = attrs[k+1];
					}
				} else {
					j++;
				}
			}
		}

		for ( n = 0; attrs[ n ] != NULL; n++ ) /* empty */;

		if ( si->si_allopattrs ) {
			attrs = ( char ** ) ch_realloc( attrs, (n + 2)*sizeof( char * ));
		} else {
			attrs = ( char ** ) ch_realloc( attrs, (n + 4)*sizeof( char * ));
		}

		if ( attrs == NULL ) {
			Debug( LDAP_DEBUG_ANY, "out of memory\n", 0, 0, 0 );
		}

		/* Add Attributes */
		if ( si->si_allopattrs ) {
			attrs[n++] = ch_strdup( sync_descs[0]->ad_cname.bv_val );
		} else {
			for ( i = 0; sync_descs[ i ] != NULL; i++ ) {
				attrs[ n++ ] = ch_strdup ( sync_descs[i]->ad_cname.bv_val );
			}
		}
		attrs[ n ] = NULL;

	} else {

		i = 0;
		if ( si->si_allattrs == si->si_allopattrs ) {
			attrs = (char**) ch_malloc( 3 * sizeof(char*) );
			attrs[i++] = ch_strdup( "*" );
			attrs[i++] = ch_strdup( "+" );
		} else if ( si->si_allattrs && !si->si_allopattrs ) {
			for ( n = 0; sync_descs[ n ] != NULL; n++ ) ;
			attrs = (char**) ch_malloc( (n+1)* sizeof(char*) );
			attrs[i++] = ch_strdup( "*" );
			for ( j = 1; sync_descs[ j ] != NULL; j++ ) {
				attrs[i++] = ch_strdup ( sync_descs[j]->ad_cname.bv_val );
			}
		} else if ( !si->si_allattrs && si->si_allopattrs ) {
			attrs = (char**) ch_malloc( 3 * sizeof(char*) );
			attrs[i++] = ch_strdup( "+" );
			attrs[i++] = ch_strdup( sync_descs[0]->ad_cname.bv_val );
		}
		attrs[i] = NULL;
	}
	
	si->si_attrs = attrs;

	exattrs = anlist2attrs( si->si_exanlist );

	if ( exattrs ) {
		for ( n = 0; exattrs[n] != NULL; n++ ) ;

		for ( i = 0; sync_descs[i] != NULL; i++ ) {
			j = 0;
			while ( exattrs[j] != NULL ) {
				if ( !strcmp( exattrs[j], sync_descs[i]->ad_cname.bv_val )) {
					ch_free( exattrs[j] );
					for ( k = j; exattrs[k] != NULL; k++ ) {
						exattrs[k] = exattrs[k+1];
					}
				} else {
					j++;
				}
			}
		}

		for ( i = 0; exattrs[i] != NULL; i++ ) {
			for ( j = 0; si->si_anlist[j].an_name.bv_val; j++ ) {
				ObjectClass	*oc;
				if ( ( oc = si->si_anlist[j].an_oc ) ) {
					k = 0;
					while ( oc->soc_required[k] ) {
						if ( !strcmp( exattrs[i],
							 oc->soc_required[k]->sat_cname.bv_val )) {
							ch_free( exattrs[i] );
							for ( l = i; exattrs[l]; l++ ) {
								exattrs[l] = exattrs[l+1];
							}
						} else {
							k++;
						}
					}
				}
			}
		}

		for ( i = 0; exattrs[i] != NULL; i++ ) ;

		if ( i != n )
			exattrs = (char **) ch_realloc( exattrs, (i + 1)*sizeof(char *));
	}

	si->si_exattrs = exattrs;	
}

typedef struct logschema {
	struct berval ls_dn;
	struct berval ls_req;
	struct berval ls_mod;
	struct berval ls_newRdn;
	struct berval ls_delRdn;
	struct berval ls_newSup;
} logschema;

static logschema changelog_sc = {
	BER_BVC("targetDN"),
	BER_BVC("changeType"),
	BER_BVC("changes"),
	BER_BVC("newRDN"),
	BER_BVC("deleteOldRDN"),
	BER_BVC("newSuperior")
};

static logschema accesslog_sc = {
	BER_BVC("reqDN"),
	BER_BVC("reqType"),
	BER_BVC("reqMod"),
	BER_BVC("reqNewRDN"),
	BER_BVC("reqDeleteOldRDN"),
	BER_BVC("reqNewSuperior")
};

static int
ldap_sync_search(
	syncinfo_t *si,
	void *ctx )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	LDAPControl c[2], *ctrls[3];
	struct timeval timeout;
	ber_int_t	msgid;
	int rc;
	int rhint;
	char *base;
	char **attrs, *lattrs[8];
	char *filter;
	int attrsonly;
	int scope;

	/* setup LDAP SYNC control */
	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &ctx );

	/* If we're using a log but we have no state, then fallback to
	 * normal mode for a full refresh.
	 */
	if ( si->si_syncdata && BER_BVISEMPTY( &si->si_syncCookie.ctxcsn ))
		si->si_logstate = SYNCLOG_FALLBACK;

	/* Use the log parameters if we're in log mode */
	if ( si->si_syncdata && si->si_logstate == SYNCLOG_LOGGING ) {
		logschema *ls;
		if ( si->si_syncdata == SYNCDATA_ACCESSLOG )
			ls = &accesslog_sc;
		else
			ls = &changelog_sc;
		lattrs[0] = ls->ls_dn.bv_val;
		lattrs[1] = ls->ls_req.bv_val;
		lattrs[2] = ls->ls_mod.bv_val;
		lattrs[3] = ls->ls_newRdn.bv_val;
		lattrs[4] = ls->ls_delRdn.bv_val;
		lattrs[5] = ls->ls_newSup.bv_val;
		lattrs[6] = slap_schema.si_ad_entryCSN->ad_cname.bv_val;
		lattrs[7] = NULL;

		rhint = 0;
		base = si->si_logbase.bv_val;
		filter = si->si_logfilterstr.bv_val;
		attrs = lattrs;
		attrsonly = 0;
		scope = LDAP_SCOPE_SUBTREE;
	} else {
		rhint = 1;
		base = si->si_base.bv_val;
		filter = si->si_filterstr.bv_val;
		attrs = si->si_attrs;
		attrsonly = si->si_attrsonly;
		scope = si->si_scope;
	}
	if ( si->si_syncdata && si->si_logstate == SYNCLOG_FALLBACK ) {
		si->si_type = LDAP_SYNC_REFRESH_ONLY;
	} else {
		si->si_type = si->si_ctype;
	}

	if ( !BER_BVISNULL( &si->si_syncCookie.octet_str ) )
	{
		ber_printf( ber, "{eOb}",
			abs(si->si_type), &si->si_syncCookie.octet_str, rhint );
	} else {
		ber_printf( ber, "{eb}",
			abs(si->si_type), rhint );
	}

	if ( (rc = ber_flatten2( ber, &c[0].ldctl_value, 0 )) == LBER_ERROR ) {
		ber_free_buf( ber );
		return rc;
	}

	c[0].ldctl_oid = LDAP_CONTROL_SYNC;
	c[0].ldctl_iscritical = si->si_type < 0;
	ctrls[0] = &c[0];

	if ( !BER_BVISNULL( &si->si_bindconf.sb_authzId ) ) {
		c[1].ldctl_oid = LDAP_CONTROL_PROXY_AUTHZ;
		c[1].ldctl_value = si->si_bindconf.sb_authzId;
		c[1].ldctl_iscritical = 1;
		ctrls[1] = &c[1];
		ctrls[2] = NULL;
	} else {
		ctrls[1] = NULL;
	}

	timeout.tv_sec = si->si_tlimit;
	timeout.tv_usec = 0;

	rc = ldap_search_ext( si->si_ld, base, scope, filter, attrs, attrsonly,
		ctrls, NULL, si->si_tlimit > 0 ? &timeout : NULL,
		si->si_slimit, &msgid );
	ber_free_buf( ber );
	return rc;
}

static int
do_syncrep1(
	Operation *op,
	syncinfo_t *si )
{
	int	rc;
	int cmdline_cookie_found = 0;

	struct sync_cookie	*sc = NULL;
	struct berval	*psub;
#ifdef HAVE_TLS
	void	*ssl;
#endif

	psub = &si->si_be->be_nsuffix[0];

	/* Init connection to master */
	rc = ldap_initialize( &si->si_ld, si->si_bindconf.sb_uri.bv_val );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"do_syncrep1: rid %03d ldap_initialize failed (%s)\n",
			si->si_rid, si->si_bindconf.sb_uri.bv_val, 0 );
		return rc;
	}

	op->o_protocol = LDAP_VERSION3;
	ldap_set_option( si->si_ld, LDAP_OPT_PROTOCOL_VERSION, &op->o_protocol );

	/* Bind to master */

	if ( si->si_bindconf.sb_tls ) {
		rc = ldap_start_tls_s( si->si_ld, NULL, NULL );
		if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"%s: rid %03d ldap_start_tls failed (%d)\n",
				si->si_bindconf.sb_tls == SB_TLS_CRITICAL ? "Error" : "Warning",
				si->si_rid, rc );
			if( si->si_bindconf.sb_tls == SB_TLS_CRITICAL ) goto done;
		}
	}

	if ( si->si_bindconf.sb_method == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
		void *defaults;

		if ( si->si_bindconf.sb_secprops != NULL ) {
			rc = ldap_set_option( si->si_ld,
				LDAP_OPT_X_SASL_SECPROPS, si->si_bindconf.sb_secprops);

			if( rc != LDAP_OPT_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "Error: rid %03d ldap_set_option "
					"(%s,SECPROPS,\"%s\") failed!\n",
					si->si_rid, si->si_bindconf.sb_uri.bv_val, si->si_bindconf.sb_secprops );
				goto done;
			}
		}

		defaults = lutil_sasl_defaults( si->si_ld,
			si->si_bindconf.sb_saslmech.bv_val,
			si->si_bindconf.sb_realm.bv_val,
			si->si_bindconf.sb_authcId.bv_val,
			si->si_bindconf.sb_cred.bv_val,
			si->si_bindconf.sb_authzId.bv_val );

		rc = ldap_sasl_interactive_bind_s( si->si_ld,
				si->si_bindconf.sb_binddn.bv_val,
				si->si_bindconf.sb_saslmech.bv_val,
				NULL, NULL,
				LDAP_SASL_QUIET,
				lutil_sasl_interact,
				defaults );

		lutil_sasl_freedefs( defaults );

		/* FIXME: different error behaviors according to
		 *	1) return code
		 *	2) on err policy : exit, retry, backoff ...
		 */
		if ( rc != LDAP_SUCCESS ) {
			static struct berval bv_GSSAPI = BER_BVC( "GSSAPI" );

			Debug( LDAP_DEBUG_ANY, "do_syncrep1: rid %03d "
				"ldap_sasl_interactive_bind_s failed (%d)\n",
				si->si_rid, rc, 0 );

			/* FIXME (see above comment) */
			/* if Kerberos credentials cache is not active, retry */
			if ( ber_bvcmp( &si->si_bindconf.sb_saslmech, &bv_GSSAPI ) == 0 &&
				rc == LDAP_LOCAL_ERROR )
			{
				rc = LDAP_SERVER_DOWN;
			}

			goto done;
		}
#else /* HAVE_CYRUS_SASL */
		/* Should never get here, we trapped this at config time */
		assert(0);
		Debug( LDAP_DEBUG_SYNC, "not compiled with SASL support\n", 0, 0, 0 );
		rc = LDAP_OTHER;
		goto done;
#endif

	} else if ( si->si_bindconf.sb_method == LDAP_AUTH_SIMPLE ) {
		rc = ldap_sasl_bind_s( si->si_ld,
			si->si_bindconf.sb_binddn.bv_val, LDAP_SASL_SIMPLE,
			&si->si_bindconf.sb_cred, NULL, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "do_syncrep1: rid %03d "
				"ldap_sasl_bind_s failed (%d)\n", si->si_rid, rc, 0 );
			goto done;
		}
	}

	/* Set SSF to strongest of TLS, SASL SSFs */
	op->o_sasl_ssf = 0;
	op->o_tls_ssf = 0;
	op->o_transport_ssf = 0;
#ifdef HAVE_TLS
	if ( ldap_get_option( si->si_ld, LDAP_OPT_X_TLS_SSL_CTX, &ssl )
		== LDAP_SUCCESS && ssl != NULL )
	{
		op->o_tls_ssf = ldap_pvt_tls_get_strength( ssl );
	}
#endif /* HAVE_TLS */
	ldap_get_option( si->si_ld, LDAP_OPT_X_SASL_SSF, &op->o_sasl_ssf );
	op->o_ssf = ( op->o_sasl_ssf > op->o_tls_ssf )
		?  op->o_sasl_ssf : op->o_tls_ssf;


	if ( BER_BVISNULL( &si->si_syncCookie.octet_str )) {
		/* get contextCSN shadow replica from database */
		BerVarray csn = NULL;

		assert( si->si_rid < 1000 );
		op->o_req_ndn = op->o_bd->be_nsuffix[0];
		op->o_req_dn = op->o_req_ndn;

		/* try to read stored contextCSN */
		backend_attribute( op, NULL, &op->o_req_ndn,
			slap_schema.si_ad_contextCSN, &csn, ACL_READ );
		if ( csn ) {
			ch_free( si->si_syncCookie.ctxcsn.bv_val );
			ber_dupbv( &si->si_syncCookie.ctxcsn, csn );
			ber_bvarray_free_x( csn, op->o_tmpmemctx );
		}

		si->si_syncCookie.rid = si->si_rid;

		LDAP_STAILQ_FOREACH( sc, &slap_sync_cookie, sc_next ) {
			if ( si->si_rid == sc->rid ) {
				cmdline_cookie_found = 1;
				break;
			}
		}

		if ( cmdline_cookie_found ) {
			/* cookie is supplied in the command line */

			LDAP_STAILQ_REMOVE( &slap_sync_cookie, sc, sync_cookie, sc_next );

			/* ctxcsn wasn't parsed yet, do it now */
			slap_parse_sync_cookie( sc, op->o_tmpmemctx );
			if ( BER_BVISNULL( &sc->ctxcsn ) ) {
				/* if cmdline cookie does not have ctxcsn */
				/* component, set it to an initial value */
				slap_init_sync_cookie_ctxcsn( sc );
			}
			slap_sync_cookie_free( &si->si_syncCookie, 0 );
			slap_dup_sync_cookie( &si->si_syncCookie, sc );
			slap_sync_cookie_free( sc, 1 );
		}

		slap_compose_sync_cookie( NULL, &si->si_syncCookie.octet_str,
			&si->si_syncCookie.ctxcsn, si->si_syncCookie.rid );
	}

	rc = ldap_sync_search( si, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "do_syncrep1: rid %03d "
			"ldap_search_ext: %s (%d)\n",
			si->si_rid, ldap_err2string( rc ), rc );
	}

done:
	if ( rc ) {
		if ( si->si_ld ) {
			ldap_unbind_ext( si->si_ld, NULL, NULL );
			si->si_ld = NULL;
		}
	}

	return rc;
}

static int
do_syncrep2(
	Operation *op,
	syncinfo_t *si )
{
	LDAPControl	**rctrls = NULL;
	LDAPControl	*rctrlp;

	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *)&berbuf;

	LDAPMessage	*res = NULL;
	LDAPMessage	*msg = NULL;

	char		*retoid = NULL;
	struct berval	*retdata = NULL;

	Entry		*entry = NULL;

	int		syncstate;
	struct berval	syncUUID = BER_BVNULL;
	struct sync_cookie	syncCookie = { BER_BVNULL };
	struct sync_cookie	syncCookie_req = { BER_BVNULL };
	struct berval		cookie = BER_BVNULL;

	int	rc, err, i;
	ber_len_t	len;

	int rc_efree = 1;

	struct berval	*psub;
	Modifications	*modlist = NULL;

	const char		*text;
	int				match;

	struct timeval *tout_p = NULL;
	struct timeval tout = { 0, 0 };

	int		refreshDeletes = 0;
	int		refreshDone = 1;
	BerVarray syncUUIDs = NULL;
	ber_tag_t si_tag;

	if ( slapd_shutdown ) {
		rc = -2;
		goto done;
	}

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE, "=>do_syncrep2 rid %03d\n", si->si_rid, 0, 0 );

	psub = &si->si_be->be_nsuffix[0];

	slap_dup_sync_cookie( &syncCookie_req, &si->si_syncCookie );

	if ( abs(si->si_type) == LDAP_SYNC_REFRESH_AND_PERSIST ) {
		tout_p = &tout;
	} else {
		tout_p = NULL;
	}

	while (( rc = ldap_result( si->si_ld, LDAP_RES_ANY, LDAP_MSG_ONE,
		tout_p, &res )) > 0 )
	{
		if ( slapd_shutdown ) {
			rc = -2;
			goto done;
		}
		for( msg = ldap_first_message( si->si_ld, res );
			msg != NULL;
			msg = ldap_next_message( si->si_ld, msg ) )
		{
			if ( slapd_shutdown ) {
				rc = -2;
				goto done;
			}
			switch( ldap_msgtype( msg ) ) {
			case LDAP_RES_SEARCH_ENTRY:
				ldap_get_entry_controls( si->si_ld, msg, &rctrls );
				/* we can't work without the control */
				if ( !rctrls ) {
					Debug( LDAP_DEBUG_ANY, "do_syncrep2: rid %03d"
						"got search entry without "
						"control\n", si->si_rid, 0, 0 );
					rc = -1;
					goto done;
				}
				rctrlp = *rctrls;
				ber_init2( ber, &rctrlp->ldctl_value, LBER_USE_DER );
				ber_scanf( ber, "{em" /*"}"*/, &syncstate, &syncUUID );
				/* FIXME: what if syncUUID is NULL or empty?
				 * (happens with back-sql...) */
				if ( BER_BVISEMPTY( &syncUUID ) ) {
					Debug( LDAP_DEBUG_ANY, "do_syncrep2: rid %03d "
						"got empty syncUUID\n", si->si_rid, 0, 0 );
					ldap_controls_free( rctrls );
					rc = -1;
					goto done;
				}
				if ( ber_peek_tag( ber, &len ) == LDAP_TAG_SYNC_COOKIE ) {
					ber_scanf( ber, /*"{"*/ "m}", &cookie );
					if ( !BER_BVISNULL( &cookie ) ) {
						ch_free( syncCookie.octet_str.bv_val );
						ber_dupbv( &syncCookie.octet_str, &cookie );
					}
					if ( !BER_BVISNULL( &syncCookie.octet_str ) )
					{
						slap_parse_sync_cookie( &syncCookie, NULL );
					}
				}
				if ( si->si_syncdata && si->si_logstate == SYNCLOG_LOGGING ) {
					entry = NULL;
					modlist = NULL;
					if ( syncrepl_message_to_op( si, op, msg ) == LDAP_SUCCESS &&
						!BER_BVISNULL( &syncCookie.ctxcsn ) ) {
						syncrepl_updateCookie( si, op, psub, &syncCookie );
					}
				} else if ( syncrepl_message_to_entry( si, op, msg,
					&modlist, &entry, syncstate ) == LDAP_SUCCESS ) {
					rc_efree = syncrepl_entry( si, op, entry, &modlist,
						syncstate, &syncUUID, &syncCookie_req, &syncCookie.ctxcsn );
					if ( !BER_BVISNULL( &syncCookie.ctxcsn ) )
					{
						syncrepl_updateCookie( si, op, psub, &syncCookie );
					}
				}
				ldap_controls_free( rctrls );
				if ( modlist ) {
					slap_mods_free( modlist, 1 );
				}
				if ( rc_efree && entry ) {
					entry_free( entry );
				}
				entry = NULL;
				break;

			case LDAP_RES_SEARCH_REFERENCE:
				Debug( LDAP_DEBUG_ANY,
					"do_syncrep2: rid %03d reference received error\n",
					si->si_rid, 0, 0 );
				break;

			case LDAP_RES_SEARCH_RESULT:
				Debug( LDAP_DEBUG_SYNC,
					"do_syncrep2: rid %03d LDAP_RES_SEARCH_RESULT\n",
					si->si_rid, 0, 0 );
				ldap_parse_result( si->si_ld, msg, &err, NULL, NULL, NULL,
					&rctrls, 0 );
#ifdef LDAP_X_SYNC_REFRESH_REQUIRED
				if ( err == LDAP_X_SYNC_REFRESH_REQUIRED ) {
					/* map old result code to registered code */
					err = LDAP_SYNC_REFRESH_REQUIRED;
				}
#endif
				if ( err == LDAP_SYNC_REFRESH_REQUIRED ) {
					if ( si->si_logstate == SYNCLOG_LOGGING ) {
						si->si_logstate = SYNCLOG_FALLBACK;
					}
					rc = err;
					goto done;
				}
				if ( rctrls ) {
					rctrlp = *rctrls;
					ber_init2( ber, &rctrlp->ldctl_value, LBER_USE_DER );

					ber_scanf( ber, "{" /*"}"*/);
					if ( ber_peek_tag( ber, &len ) == LDAP_TAG_SYNC_COOKIE ) {
						ber_scanf( ber, "m", &cookie );
						if ( !BER_BVISNULL( &cookie ) ) {
							ch_free( syncCookie.octet_str.bv_val );
							ber_dupbv( &syncCookie.octet_str, &cookie);
						}
						if ( !BER_BVISNULL( &syncCookie.octet_str ) )
						{
							slap_parse_sync_cookie( &syncCookie, NULL );
						}
					}
					if ( ber_peek_tag( ber, &len ) == LDAP_TAG_REFRESHDELETES )
					{
						ber_scanf( ber, "b", &refreshDeletes );
					}
					ber_scanf( ber, /*"{"*/ "}" );
				}
				if ( BER_BVISNULL( &syncCookie_req.ctxcsn )) {
					match = -1;
				} else if ( BER_BVISNULL( &syncCookie.ctxcsn )) {
					match = 1;
				} else {
					value_match( &match, slap_schema.si_ad_entryCSN,
						slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
						SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
						&syncCookie_req.ctxcsn, &syncCookie.ctxcsn,
						&text );
				}
				if ( rctrls ) {
					ldap_controls_free( rctrls );
				}
				if (si->si_type != LDAP_SYNC_REFRESH_AND_PERSIST) {
					/* FIXME : different error behaviors according to
					 *	1) err code : LDAP_BUSY ...
					 *	2) on err policy : stop service, stop sync, retry
					 */
					if ( refreshDeletes == 0 && match < 0 &&
						err == LDAP_SUCCESS )
					{
						syncrepl_del_nonpresent( op, si, NULL, &syncCookie.ctxcsn );
					} else {
						avl_free( si->si_presentlist, avl_ber_bvfree );
						si->si_presentlist = NULL;
					}
				}
				if ( !BER_BVISNULL( &syncCookie.ctxcsn ) &&
					match < 0 && err == LDAP_SUCCESS )
				{
					syncrepl_updateCookie( si, op, psub, &syncCookie );
				}
				if ( err == LDAP_SUCCESS
					&& si->si_logstate == SYNCLOG_FALLBACK ) {
					si->si_logstate = SYNCLOG_LOGGING;
					rc = LDAP_SYNC_REFRESH_REQUIRED;
				} else {
					rc = -2;
				}
				goto done;
				break;

			case LDAP_RES_INTERMEDIATE:
				rc = ldap_parse_intermediate( si->si_ld, msg,
					&retoid, &retdata, NULL, 0 );
				if ( !rc && !strcmp( retoid, LDAP_SYNC_INFO ) ) {
					ber_init2( ber, retdata, LBER_USE_DER );

					switch ( si_tag = ber_peek_tag( ber, &len )) {
					ber_tag_t tag;
					case LDAP_TAG_SYNC_NEW_COOKIE:
						Debug( LDAP_DEBUG_SYNC,
							"do_syncrep2: rid %03d %s - %s\n", 
							si->si_rid,
							"LDAP_RES_INTERMEDIATE", 
							"NEW_COOKIE" );
						ber_scanf( ber, "tm", &tag, &cookie );
						break;
					case LDAP_TAG_SYNC_REFRESH_DELETE:
					case LDAP_TAG_SYNC_REFRESH_PRESENT:
						Debug( LDAP_DEBUG_SYNC,
							"do_syncrep2: rid %03d %s - %s\n", 
							si->si_rid,
							"LDAP_RES_INTERMEDIATE", 
							si_tag == LDAP_TAG_SYNC_REFRESH_PRESENT ?
							"REFRESH_PRESENT" : "REFRESH_DELETE" );
						if ( si_tag == LDAP_TAG_SYNC_REFRESH_DELETE ) {
							si->si_refreshDelete = 1;
						} else {
							si->si_refreshPresent = 1;
						}
						ber_scanf( ber, "t{" /*"}"*/, &tag );
						if ( ber_peek_tag( ber, &len ) == LDAP_TAG_SYNC_COOKIE )
						{
							ber_scanf( ber, "m", &cookie );
							if ( !BER_BVISNULL( &cookie ) ) {
								ch_free( syncCookie.octet_str.bv_val );
								ber_dupbv( &syncCookie.octet_str, &cookie );
							}
							if ( !BER_BVISNULL( &syncCookie.octet_str ) )
							{
								slap_parse_sync_cookie( &syncCookie, NULL );
							}
						}
						if ( ber_peek_tag( ber, &len ) ==
							LDAP_TAG_REFRESHDONE )
						{
							ber_scanf( ber, "b", &refreshDone );
						}
						ber_scanf( ber, /*"{"*/ "}" );
						break;
					case LDAP_TAG_SYNC_ID_SET:
						Debug( LDAP_DEBUG_SYNC,
							"do_syncrep2: rid %03d %s - %s\n", 
							si->si_rid,
							"LDAP_RES_INTERMEDIATE", 
							"SYNC_ID_SET" );
						ber_scanf( ber, "t{" /*"}"*/, &tag );
						if ( ber_peek_tag( ber, &len ) ==
							LDAP_TAG_SYNC_COOKIE )
						{
							ber_scanf( ber, "m", &cookie );
							if ( !BER_BVISNULL( &cookie ) ) {
								ch_free( syncCookie.octet_str.bv_val );
								ber_dupbv( &syncCookie.octet_str, &cookie );
							}
							if ( !BER_BVISNULL( &syncCookie.octet_str ) )
							{
								slap_parse_sync_cookie( &syncCookie, NULL );
							}
						}
						if ( ber_peek_tag( ber, &len ) ==
							LDAP_TAG_REFRESHDELETES )
						{
							ber_scanf( ber, "b", &refreshDeletes );
						}
						ber_scanf( ber, "[W]", &syncUUIDs );
						ber_scanf( ber, /*"{"*/ "}" );
						if ( refreshDeletes ) {
							syncrepl_del_nonpresent( op, si, syncUUIDs,
								&syncCookie.ctxcsn );
							ber_bvarray_free_x( syncUUIDs, op->o_tmpmemctx );
						} else {
							for ( i = 0; !BER_BVISNULL( &syncUUIDs[i] ); i++ ) {
								struct berval *syncuuid_bv;
								syncuuid_bv = ber_dupbv( NULL, &syncUUIDs[i] );
								slap_sl_free( syncUUIDs[i].bv_val,op->o_tmpmemctx );
								avl_insert( &si->si_presentlist,
									(caddr_t) syncuuid_bv,
									syncuuid_cmp, avl_dup_error );
							}
							slap_sl_free( syncUUIDs, op->o_tmpmemctx );
						}
						slap_sync_cookie_free( &syncCookie, 0 );
						break;
					default:
						Debug( LDAP_DEBUG_ANY,
							"do_syncrep2: rid %03d unknown syncinfo tag (%ld)\n",
							si->si_rid, (long) si_tag, 0 );
						ldap_memfree( retoid );
						ber_bvfree( retdata );
						continue;
					}

					if ( BER_BVISNULL( &syncCookie_req.ctxcsn )) {
						match = -1;
					} else if ( BER_BVISNULL( &syncCookie.ctxcsn )) {
						match = 1;
					} else {
						value_match( &match, slap_schema.si_ad_entryCSN,
							slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							&syncCookie_req.ctxcsn,
							&syncCookie.ctxcsn, &text );
					}

					if ( match < 0 ) {
						if ( si->si_refreshPresent == 1 ) {
							syncrepl_del_nonpresent( op, si, NULL, &syncCookie.ctxcsn );
						}
						if ( !BER_BVISNULL( &syncCookie.ctxcsn ))
						{
							syncrepl_updateCookie( si, op, psub, &syncCookie);
						}
					} 

					ldap_memfree( retoid );
					ber_bvfree( retdata );
					break;

				} else {
					Debug( LDAP_DEBUG_ANY, "do_syncrep2: rid %03d "
						"unknown intermediate response (%d)\n",
						si->si_rid, rc, 0 );
					ldap_memfree( retoid );
					ber_bvfree( retdata );
					break;
				}
				break;

			default:
				Debug( LDAP_DEBUG_ANY, "do_syncrep2: rid %03d "
					"unknown message\n", si->si_rid, 0, 0 );
				break;

			}
			if ( !BER_BVISNULL( &syncCookie.octet_str )) {
				slap_sync_cookie_free( &syncCookie_req, 0 );
				slap_dup_sync_cookie( &syncCookie_req, &syncCookie );
				slap_sync_cookie_free( &syncCookie, 0 );
			}
		}
		ldap_msgfree( res );
		res = NULL;
	}

	if ( rc == -1 ) {
		const char *errstr;

		ldap_get_option( si->si_ld, LDAP_OPT_ERROR_NUMBER, &rc );
		errstr = ldap_err2string( rc );
		
		Debug( LDAP_DEBUG_ANY,
			"do_syncrep2: rid %03d %s\n", si->si_rid, errstr, 0 );
	}

done:
	slap_sync_cookie_free( &syncCookie, 0 );
	slap_sync_cookie_free( &syncCookie_req, 0 );

	if ( res ) ldap_msgfree( res );

	if ( rc && rc != LDAP_SYNC_REFRESH_REQUIRED && si->si_ld ) {
		if ( si->si_conn_setup ) {
			ber_socket_t s;
			ldap_get_option( si->si_ld, LDAP_OPT_DESC, &s );
			connection_client_stop( s );
			si->si_conn_setup = 0;
		}
		ldap_unbind_ext( si->si_ld, NULL, NULL );
		si->si_ld = NULL;
	}

	return rc;
}

static void *
do_syncrepl(
	void	*ctx,
	void	*arg )
{
	struct re_s* rtask = arg;
	syncinfo_t *si = ( syncinfo_t * ) rtask->arg;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	int rc = LDAP_SUCCESS;
	int dostop = 0;
	ber_socket_t s;
	int i, defer = 1, fail = 0;
	Backend *be;

	Debug( LDAP_DEBUG_TRACE, "=>do_syncrepl rid %03d\n", si->si_rid, 0, 0 );

	if ( si == NULL )
		return NULL;

	ldap_pvt_thread_mutex_lock( &si->si_mutex );

	switch( abs( si->si_type )) {
	case LDAP_SYNC_REFRESH_ONLY:
	case LDAP_SYNC_REFRESH_AND_PERSIST:
		break;
	default:
		ldap_pvt_thread_mutex_unlock( &si->si_mutex );
		return NULL;
	}

	if ( slapd_shutdown ) {
		if ( si->si_ld ) {
			if ( si->si_conn_setup ) {
				ldap_get_option( si->si_ld, LDAP_OPT_DESC, &s );
				connection_client_stop( s );
				si->si_conn_setup = 0;
			}
			ldap_unbind_ext( si->si_ld, NULL, NULL );
			si->si_ld = NULL;
		}
		ldap_pvt_thread_mutex_unlock( &si->si_mutex );
		return NULL;
	}

	op = (Operation *) &opbuf;
	connection_fake_init( &conn, op, ctx );

	/* use global malloc for now */
	op->o_tmpmemctx = NULL;
	op->o_tmpmfuncs = &ch_mfuncs;

	op->o_managedsait = SLAP_CONTROL_NONCRITICAL;
	op->o_bd = be = si->si_be;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;

	/* Establish session, do search */
	if ( !si->si_ld ) {
		si->si_refreshDelete = 0;
		si->si_refreshPresent = 0;
		rc = do_syncrep1( op, si );
	}

reload:
	/* Process results */
	if ( rc == LDAP_SUCCESS ) {
		ldap_get_option( si->si_ld, LDAP_OPT_DESC, &s );

		rc = do_syncrep2( op, si );
		if ( rc == LDAP_SYNC_REFRESH_REQUIRED )	{
			rc = ldap_sync_search( si, op->o_tmpmemctx );
			goto reload;
		}

		if ( abs(si->si_type) == LDAP_SYNC_REFRESH_AND_PERSIST ) {
			/* If we succeeded, enable the connection for further listening.
			 * If we failed, tear down the connection and reschedule.
			 */
			if ( rc == LDAP_SUCCESS ) {
				if ( si->si_conn_setup ) {
					connection_client_enable( s );
				} else {
					rc = connection_client_setup( s, do_syncrepl, arg );
					if ( rc == 0 )
						si->si_conn_setup = 1;
				} 
			} else if ( si->si_conn_setup ) {
				dostop = 1;
			}
		} else {
			if ( rc == -2 ) rc = 0;
		}
	}

	/* At this point, we have 4 cases:
	 * 1) for any hard failure, give up and remove this task
	 * 2) for ServerDown, reschedule this task to run
	 * 3) for Refresh and Success, reschedule to run
	 * 4) for Persist and Success, reschedule to defer
	 */
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );

	if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask )) {
		ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	}

	if ( dostop ) {
		connection_client_stop( s );
	}

	if ( rc == LDAP_SUCCESS ) {
		if ( si->si_type == LDAP_SYNC_REFRESH_ONLY ) {
			defer = 0;
		}
		rtask->interval.tv_sec = si->si_interval;
		ldap_pvt_runqueue_resched( &slapd_rq, rtask, defer );
		if ( si->si_retrynum ) {
			for ( i = 0; si->si_retrynum_init[i] != RETRYNUM_TAIL; i++ ) {
				si->si_retrynum[i] = si->si_retrynum_init[i];
			}
			si->si_retrynum[i] = RETRYNUM_TAIL;
		}
	} else {
		for ( i = 0; si->si_retrynum && si->si_retrynum[i] <= 0; i++ ) {
			if ( si->si_retrynum[i] == RETRYNUM_FOREVER || si->si_retrynum[i] == RETRYNUM_TAIL )
				break;
		}

		if ( !si->si_retrynum || si->si_retrynum[i] == RETRYNUM_TAIL ) {
			ldap_pvt_runqueue_remove( &slapd_rq, rtask );
			fail = RETRYNUM_TAIL;
		} else if ( RETRYNUM_VALID( si->si_retrynum[i] ) ) {
			if ( si->si_retrynum[i] > 0 )
				si->si_retrynum[i]--;
			fail = si->si_retrynum[i];
			rtask->interval.tv_sec = si->si_retryinterval[i];
			ldap_pvt_runqueue_resched( &slapd_rq, rtask, 0 );
			slap_wake_listener();
		}
	}

	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	ldap_pvt_thread_mutex_unlock( &si->si_mutex );

	if ( rc ) {
		if ( fail == RETRYNUM_TAIL ) {
			Debug( LDAP_DEBUG_ANY,
				"do_syncrepl: rid %03d quitting\n",
				si->si_rid, 0, 0 );
		} else if ( fail > 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"do_syncrepl: rid %03d retrying (%d retries left)\n",
				si->si_rid, fail, 0 );
		} else {
			Debug( LDAP_DEBUG_ANY,
				"do_syncrepl: rid %03d retrying\n",
				si->si_rid, 0, 0 );
		}
	}

	return NULL;
}

static slap_verbmasks modops[] = {
	{ BER_BVC("add"), LDAP_REQ_ADD },
	{ BER_BVC("delete"), LDAP_REQ_DELETE },
	{ BER_BVC("modify"), LDAP_REQ_MODIFY },
	{ BER_BVC("modrdn"), LDAP_REQ_MODRDN},
	{ BER_BVNULL, 0 }
};

static Modifications *
syncrepl_accesslog_mods(
	syncinfo_t *si,
	struct berval *vals
)
{
	char *colon;
	const char *text;
	AttributeDescription *ad;
	struct berval bv, bv2;
	short op;
	Modifications *mod = NULL, *modlist = NULL, **modtail;
	int i;

	modtail = &modlist;

	for (i=0; !BER_BVISNULL( &vals[i] ); i++) {
		ad = NULL;
		bv = vals[i];

		colon = ber_bvchr( &bv, ':' );
		if ( !colon )
			continue;	/* invalid */
		bv.bv_len = colon - bv.bv_val;
		if ( slap_bv2ad( &bv, &ad, &text )) {
			/* Invalid */
			continue;
		}
		/* Ignore dynamically generated attrs */
		if ( ad->ad_type->sat_flags & SLAP_AT_DYNAMIC )
			continue;
		/* Ignore excluded attrs */
		if ( ldap_charray_inlist( si->si_exattrs,
			ad->ad_type->sat_cname.bv_val ))
			continue;

		switch(colon[1]) {
		case '+':	op = LDAP_MOD_ADD; break;
		case '-':	op = LDAP_MOD_DELETE; break;
		case '=':	op = LDAP_MOD_REPLACE; break;
		case '#':	op = LDAP_MOD_INCREMENT; break;
		default:	continue;
		}

		if ( !mod || ad != mod->sml_desc || op != mod->sml_op ) {
			mod = (Modifications *) ch_malloc( sizeof( Modifications ));
			mod->sml_flags = 0;
			mod->sml_op = op;
			mod->sml_next = NULL;
			mod->sml_desc = ad;
			mod->sml_type = ad->ad_cname;
			mod->sml_values = NULL;
			mod->sml_nvalues = NULL;

			*modtail = mod;
			modtail = &mod->sml_next;
		}
		if ( colon[2] == ' ' ) {
			bv.bv_val = colon + 3;
			bv.bv_len = vals[i].bv_len - ( bv.bv_val - vals[i].bv_val );
			ber_dupbv( &bv2, &bv );
			ber_bvarray_add( &mod->sml_values, &bv2 );
		}
	}
	return modlist;
}

static Modifications *
syncrepl_changelog_mods(
	syncinfo_t *si,
	struct berval *vals
)
{
	return NULL;	/* FIXME */
}

static int
syncrepl_message_to_op(
	syncinfo_t	*si,
	Operation	*op,
	LDAPMessage	*msg
)
{
	BerElement	*ber = NULL;
	Modifications	*modlist = NULL;
	logschema *ls;
	SlapReply rs = { REP_RESULT };
	slap_callback cb = { NULL, null_callback, NULL, NULL };

	const char	*text;
	char txtbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof txtbuf;

	struct berval	bdn, dn = BER_BVNULL, ndn;
	struct berval	bv, *bvals = NULL;
	struct berval	rdn = BER_BVNULL, sup = BER_BVNULL,
		prdn = BER_BVNULL, nrdn = BER_BVNULL,
		psup = BER_BVNULL, nsup = BER_BVNULL;
	int		rc, deleteOldRdn = 0, freeReqDn = 0;

	if ( ldap_msgtype( msg ) != LDAP_RES_SEARCH_ENTRY ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_op: rid %03d "
			"Message type should be entry (%d)",
			si->si_rid, ldap_msgtype( msg ), 0 );
		return -1;
	}

	if ( si->si_syncdata == SYNCDATA_ACCESSLOG )
		ls = &accesslog_sc;
	else
		ls = &changelog_sc;

	rc = ldap_get_dn_ber( si->si_ld, msg, &ber, &bdn );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_message_to_op: rid %03d dn get failed (%d)",
			si->si_rid, rc, 0 );
		return rc;
	}

	op->o_tag = LBER_DEFAULT;

	while (( rc = ldap_get_attribute_ber( si->si_ld, msg, ber, &bv, &bvals ))
		== LDAP_SUCCESS ) {
		if ( bv.bv_val == NULL )
			break;

		if ( !ber_bvstrcasecmp( &bv, &ls->ls_dn )) {
			bdn = bvals[0];
			dnPrettyNormal( NULL, &bdn, &dn, &ndn, op->o_tmpmemctx );
			ber_dupbv( &op->o_req_dn, &dn );
			ber_dupbv( &op->o_req_ndn, &ndn );
			slap_sl_free( ndn.bv_val, op->o_tmpmemctx );
			slap_sl_free( dn.bv_val, op->o_tmpmemctx );
			freeReqDn = 1;
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_req )) {
			int i = verb_to_mask( bvals[0].bv_val, modops );
			if ( i < 0 ) {
				Debug( LDAP_DEBUG_ANY,
					"syncrepl_message_to_op: rid %03d unknown op %s",
					si->si_rid, bvals[0].bv_val, 0 );
				ch_free( bvals );
				rc = -1;
				goto done;
			}
			op->o_tag = modops[i].mask;
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_mod )) {
			/* Parse attribute into modlist */
			if ( si->si_syncdata == SYNCDATA_ACCESSLOG )
				modlist = syncrepl_accesslog_mods( si, bvals );
			else
				modlist = syncrepl_changelog_mods( si, bvals );
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_newRdn )) {
			rdn = bvals[0];
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_delRdn )) {
			if ( !ber_bvstrcasecmp( &slap_true_bv, bvals ))
				deleteOldRdn = 1;
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_newSup )) {
			sup = bvals[0];
		} else if ( !ber_bvstrcasecmp( &bv,
			&slap_schema.si_ad_entryCSN->ad_cname )) {
			slap_queue_csn( op, bvals );
		}
		ch_free( bvals );
	}

	/* If we didn't get a mod type or a target DN, bail out */
	if ( op->o_tag == LBER_DEFAULT || BER_BVISNULL( &dn )) {
		rc = -1;
		goto done;
	}

	op->o_callback = &cb;
	slap_op_time( &op->o_time, &op->o_tincr );

	switch( op->o_tag ) {
	case LDAP_REQ_ADD:
	case LDAP_REQ_MODIFY:
		/* If we didn't get required data, bail */
		if ( !modlist ) goto done;

		rc = slap_mods_check( modlist, &text, txtbuf, textlen, NULL );

		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_op: rid %03d "
				"mods check (%s)\n",
				si->si_rid, text, 0 );
			goto done;
		}

		if ( op->o_tag == LDAP_REQ_ADD ) {
			op->ora_e = ( Entry * ) ch_calloc( 1, sizeof( Entry ) );
			op->ora_e->e_name = op->o_req_dn;
			op->ora_e->e_nname = op->o_req_ndn;
			freeReqDn = 0;
			rc = slap_mods2entry( modlist, &op->ora_e, 1, 0, &text, txtbuf, textlen);
			if( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_op: rid %03d "
				"mods2entry (%s)\n",
					si->si_rid, text, 0 );
			} else {
				rc = op->o_bd->be_add( op, &rs );
				Debug( LDAP_DEBUG_SYNC,
					"syncrepl_message_to_op: rid %03d be_add %s (%d)\n", 
					si->si_rid, op->o_req_dn.bv_val, rc );
			}
			be_entry_release_w( op, op->ora_e );
		} else {
			op->orm_modlist = modlist;
			rc = op->o_bd->be_modify( op, &rs );
			modlist = op->orm_modlist;
			Debug( rc ? LDAP_DEBUG_ANY : LDAP_DEBUG_SYNC,
				"syncrepl_message_to_op: rid %03d be_modify %s (%d)\n", 
				si->si_rid, op->o_req_dn.bv_val, rc );
		}
		break;
	case LDAP_REQ_MODRDN:
		if ( BER_BVISNULL( &rdn )) goto done;

		if ( rdnPretty( NULL, &rdn, &prdn, NULL ))
			goto done;
		if ( rdnNormalize( 0, NULL, NULL, &rdn, &nrdn, NULL ))
			goto done;
		if ( !BER_BVISNULL( &sup )) {
			if ( dnPrettyNormal( NULL, &sup, &psup, &nsup, NULL ))
				goto done;
			op->orr_newSup = &psup;
			op->orr_nnewSup = &nsup;
		} else {
			op->orr_newSup = NULL;
			op->orr_nnewSup = NULL;
		}
		op->orr_newrdn = prdn;
		op->orr_nnewrdn = nrdn;
		op->orr_deleteoldrdn = deleteOldRdn;
		rc = op->o_bd->be_modrdn( op, &rs );
		Debug( rc ? LDAP_DEBUG_ANY : LDAP_DEBUG_SYNC,
			"syncrepl_message_to_op: rid %03d be_modrdn %s (%d)\n", 
			si->si_rid, op->o_req_dn.bv_val, rc );
		break;
	case LDAP_REQ_DELETE:
		rc = op->o_bd->be_delete( op, &rs );
		Debug( rc ? LDAP_DEBUG_ANY : LDAP_DEBUG_SYNC,
			"syncrepl_message_to_op: rid %03d be_delete %s (%d)\n", 
			si->si_rid, op->o_req_dn.bv_val, rc );
		break;
	}
done:
	slap_graduate_commit_csn( op );
	op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
	BER_BVZERO( &op->o_csn );
	if ( modlist )
		slap_mods_free( modlist, op->o_tag != LDAP_REQ_ADD );
	if ( !BER_BVISNULL( &rdn )) {
		if ( !BER_BVISNULL( &nsup ))
			ch_free( nsup.bv_val );
		if ( !BER_BVISNULL( &psup ))
			ch_free( psup.bv_val );
		if ( !BER_BVISNULL( &nrdn ))
			ch_free( nrdn.bv_val );
		if ( !BER_BVISNULL( &prdn ))
			ch_free( prdn.bv_val );
	}
	if ( freeReqDn ) {
		ch_free( op->o_req_ndn.bv_val );
		ch_free( op->o_req_dn.bv_val );
	}
	ber_free ( ber, 0 );
	return rc;
}

static int
syncrepl_message_to_entry(
	syncinfo_t	*si,
	Operation	*op,
	LDAPMessage	*msg,
	Modifications	**modlist,
	Entry			**entry,
	int		syncstate
)
{
	Entry		*e = NULL;
	BerElement	*ber = NULL;
	Modifications	tmp;
	Modifications	*mod;
	Modifications	**modtail = modlist;

	const char	*text;
	char txtbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof txtbuf;

	struct berval	bdn = BER_BVNULL, dn, ndn;
	int		rc;

	*modlist = NULL;

	if ( ldap_msgtype( msg ) != LDAP_RES_SEARCH_ENTRY ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: rid %03d "
			"Message type should be entry (%d)",
			si->si_rid, ldap_msgtype( msg ), 0 );
		return -1;
	}

	op->o_tag = LDAP_REQ_ADD;

	rc = ldap_get_dn_ber( si->si_ld, msg, &ber, &bdn );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_message_to_entry: rid %03d dn get failed (%d)",
			si->si_rid, rc, 0 );
		return rc;
	}

	dnPrettyNormal( NULL, &bdn, &dn, &ndn, op->o_tmpmemctx );
	ber_dupbv( &op->o_req_dn, &dn );
	ber_dupbv( &op->o_req_ndn, &ndn );
	slap_sl_free( ndn.bv_val, op->o_tmpmemctx );
	slap_sl_free( dn.bv_val, op->o_tmpmemctx );

	if ( syncstate == LDAP_SYNC_PRESENT || syncstate == LDAP_SYNC_DELETE ) {
		/* NOTE: this could be done even before decoding the DN,
		 * although encoding errors wouldn't be detected */
		rc = LDAP_SUCCESS;
		goto done;
	}

	if ( entry == NULL ) {
		rc = -1;
		goto done;
	}

	e = ( Entry * ) ch_calloc( 1, sizeof( Entry ) );
	*entry = e;
	e->e_name = op->o_req_dn;
	e->e_nname = op->o_req_ndn;

	while ( ber_remaining( ber ) ) {
		if ( (ber_scanf( ber, "{mW}", &tmp.sml_type, &tmp.sml_values ) ==
			LBER_ERROR ) || BER_BVISNULL( &tmp.sml_type ) )
		{
			break;
		}

		mod  = (Modifications *) ch_malloc( sizeof( Modifications ));

		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_flags = 0;
		mod->sml_next = NULL;
		mod->sml_desc = NULL;
		mod->sml_type = tmp.sml_type;
		mod->sml_values = tmp.sml_values;
		mod->sml_nvalues = NULL;

		*modtail = mod;
		modtail = &mod->sml_next;
	}

	if ( *modlist == NULL ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: rid %03d no attributes\n",
			si->si_rid, 0, 0 );
		rc = -1;
		goto done;
	}

	rc = slap_mods_check( *modlist, &text, txtbuf, textlen, NULL );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: rid %03d mods check (%s)\n",
			si->si_rid, text, 0 );
		goto done;
	}

	/* Strip out dynamically generated attrs */
	for ( modtail = modlist; *modtail ; ) {
		mod = *modtail;
		if ( mod->sml_desc->ad_type->sat_flags & SLAP_AT_DYNAMIC ) {
			*modtail = mod->sml_next;
			slap_mod_free( &mod->sml_mod, 0 );
			ch_free( mod );
		} else {
			modtail = &mod->sml_next;
		}
	}

	/* Strip out attrs in exattrs list */
	for ( modtail = modlist; *modtail ; ) {
		mod = *modtail;
		if ( ldap_charray_inlist( si->si_exattrs,
					mod->sml_desc->ad_type->sat_cname.bv_val )) {
			*modtail = mod->sml_next;
			slap_mod_free( &mod->sml_mod, 0 );
			ch_free( mod );
		} else {
			modtail = &mod->sml_next;
		}
	}
	
	rc = slap_mods2entry( *modlist, &e, 1, 1, &text, txtbuf, textlen);
	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: rid %03d mods2entry (%s)\n",
			si->si_rid, text, 0 );
	}

done:
	ber_free( ber, 0 );
	if ( rc != LDAP_SUCCESS ) {
		if ( e ) {
			entry_free( e );
			*entry = e = NULL;
		}
	}

	return rc;
}

static struct berval generic_filterstr = BER_BVC("(objectclass=*)");

/* During a refresh, we may get an LDAP_SYNC_ADD for an already existing
 * entry if a previous refresh was interrupted before sending us a new
 * context state. We try to compare the new entry to the existing entry
 * and ignore the new entry if they are the same.
 *
 * Also, we may get an update where the entryDN has changed, due to
 * a ModDn on the provider. We detect this as well, so we can issue
 * the corresponding operation locally.
 *
 * In the case of a modify, we get a list of all the attributes
 * in the original entry. Rather than deleting the entry and re-adding it,
 * we issue a Modify request that deletes all the attributes and adds all
 * the new ones. This avoids the issue of trying to delete/add a non-leaf
 * entry.
 *
 * We don't try to otherwise distinguish ModDN from Modify; in the case of
 * a ModDN we will issue both operations on the local database.
 */
typedef struct dninfo {
	Entry *new_entry;
	struct berval dn;
	struct berval ndn;
	int renamed;	/* Was an existing entry renamed? */
	int wasChanged;	/* are the attributes changed? */
	int attrs;		/* how many attribute types are in the ads list */
	AttributeDescription **ads;
} dninfo;

static int
syncrepl_entry(
	syncinfo_t* si,
	Operation *op,
	Entry* entry,
	Modifications** modlist,
	int syncstate,
	struct berval* syncUUID,
	struct sync_cookie* syncCookie_req,
	struct berval* syncCSN )
{
	Backend *be = op->o_bd;
	slap_callback	cb = { NULL, NULL, NULL, NULL };
	struct berval	*syncuuid_bv = NULL;
	struct berval	syncUUID_strrep = BER_BVNULL;
	struct berval	uuid_bv = BER_BVNULL;

	SlapReply	rs_search = {REP_RESULT};
	SlapReply	rs_delete = {REP_RESULT};
	SlapReply	rs_add = {REP_RESULT};
	SlapReply	rs_modify = {REP_RESULT};
	Filter f = {0};
#ifdef LDAP_COMP_MATCH
	AttributeAssertion ava = { NULL, BER_BVNULL, NULL };
#else
	AttributeAssertion ava = { NULL, BER_BVNULL };
#endif
	int rc = LDAP_SUCCESS;
	int ret = LDAP_SUCCESS;

	struct berval pdn = BER_BVNULL;
	dninfo dni = {0};
	int	retry = 1;

	switch( syncstate ) {
	case LDAP_SYNC_PRESENT:
		Debug( LDAP_DEBUG_SYNC, "syncrepl_entry: rid %03d %s\n",
					si->si_rid,
					"LDAP_RES_SEARCH_ENTRY(LDAP_SYNC_PRESENT)", 0 );
		break;
	case LDAP_SYNC_ADD:
		Debug( LDAP_DEBUG_SYNC, "syncrepl_entry: rid %03d %s\n",
					si->si_rid,
					"LDAP_RES_SEARCH_ENTRY(LDAP_SYNC_ADD)", 0 );
		break;
	case LDAP_SYNC_DELETE:
		Debug( LDAP_DEBUG_SYNC, "syncrepl_entry: rid %03d %s\n",
					si->si_rid,
					"LDAP_RES_SEARCH_ENTRY(LDAP_SYNC_DELETE)", 0 );
		break;
	case LDAP_SYNC_MODIFY:
		Debug( LDAP_DEBUG_SYNC, "syncrepl_entry: rid %03d %s\n",
					si->si_rid,
					"LDAP_RES_SEARCH_ENTRY(LDAP_SYNC_MODIFY)", 0 );
		break;
	default:
		Debug( LDAP_DEBUG_ANY, "syncrepl_entry: rid %03d %s\n",
					si->si_rid,
					"LDAP_RES_SEARCH_ENTRY(UNKNOWN syncstate)", 0 );
	}

	if (( syncstate == LDAP_SYNC_PRESENT || syncstate == LDAP_SYNC_ADD )) {
		if ( !si->si_refreshPresent ) {
			syncuuid_bv = ber_dupbv( NULL, syncUUID );
			avl_insert( &si->si_presentlist, (caddr_t) syncuuid_bv,
				syncuuid_cmp, avl_dup_error );
		}
	}

	if ( syncstate == LDAP_SYNC_PRESENT ) {
		return 0;
	} else if ( syncstate != LDAP_SYNC_DELETE ) {
		if ( entry == NULL ) {
			return 0;
		}
	}

	f.f_choice = LDAP_FILTER_EQUALITY;
	f.f_ava = &ava;
	ava.aa_desc = slap_schema.si_ad_entryUUID;
	(void)slap_uuidstr_from_normalized( &syncUUID_strrep, syncUUID, op->o_tmpmemctx );
	ava.aa_value = *syncUUID;
	op->ors_filter = &f;

	op->ors_filterstr.bv_len = STRLENOF( "(entryUUID=)" ) + syncUUID_strrep.bv_len;
	op->ors_filterstr.bv_val = (char *) slap_sl_malloc(
		op->ors_filterstr.bv_len + 1, op->o_tmpmemctx ); 
	AC_MEMCPY( op->ors_filterstr.bv_val, "(entryUUID=", STRLENOF( "(entryUUID=" ) );
	AC_MEMCPY( &op->ors_filterstr.bv_val[STRLENOF( "(entryUUID=" )],
		syncUUID_strrep.bv_val, syncUUID_strrep.bv_len );
	op->ors_filterstr.bv_val[op->ors_filterstr.bv_len - 1] = ')';
	op->ors_filterstr.bv_val[op->ors_filterstr.bv_len] = '\0';

	op->o_tag = LDAP_REQ_SEARCH;
	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;

	/* get the entry for this UUID */
	op->o_req_dn = si->si_base;
	op->o_req_ndn = si->si_base;

	op->o_time = slap_get_time();
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = 1;

	op->ors_attrs = slap_anlist_all_attributes;
	op->ors_attrsonly = 0;

	/* set callback function */
	op->o_callback = &cb;
	cb.sc_response = dn_callback;
	cb.sc_private = &dni;
	dni.new_entry = entry;

	if ( limits_check( op, &rs_search ) == 0 ) {
		rc = be->be_search( op, &rs_search );
		Debug( LDAP_DEBUG_SYNC,
				"syncrepl_entry: rid %03d be_search (%d)\n", 
				si->si_rid, rc, 0 );
	}

	if ( !BER_BVISNULL( &op->ors_filterstr ) ) {
		slap_sl_free( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	}

	cb.sc_response = null_callback;
	cb.sc_private = si;

	if ( entry && !BER_BVISNULL( &entry->e_name ) ) {
		Debug( LDAP_DEBUG_SYNC,
				"syncrepl_entry: rid %03d %s\n",
				si->si_rid, entry->e_name.bv_val, 0 );
	} else {
		Debug( LDAP_DEBUG_SYNC,
				"syncrepl_entry: rid %03d %s\n",
				si->si_rid, dni.dn.bv_val ? dni.dn.bv_val : "(null)", 0 );
	}

	if ( syncstate != LDAP_SYNC_DELETE ) {
		Attribute	*a = attr_find( entry->e_attrs, slap_schema.si_ad_entryUUID );

		if ( a == NULL ) {
			/* add if missing */
			attr_merge_one( entry, slap_schema.si_ad_entryUUID,
				&syncUUID_strrep, syncUUID );

		} else if ( !bvmatch( &a->a_nvals[0], syncUUID ) ) {
			/* replace only if necessary */
			if ( a->a_nvals != a->a_vals ) {
				ber_memfree( a->a_nvals[0].bv_val );
				ber_dupbv( &a->a_nvals[0], syncUUID );
			}
			ber_memfree( a->a_vals[0].bv_val );
			ber_dupbv( &a->a_vals[0], &syncUUID_strrep );
		}
		/* Don't save the contextCSN on the inooming context entry,
		 * we'll write it when syncrepl_updateCookie eventually
		 * gets called. (ITS#4622)
		 */
		if ( syncstate == LDAP_SYNC_ADD && dn_match( &entry->e_nname,
			&be->be_nsuffix[0] )) {
			Attribute **ap;
			for ( ap = &entry->e_attrs; *ap; ap=&(*ap)->a_next ) {
				a = *ap;
				if ( a->a_desc == slap_schema.si_ad_contextCSN ) {
					*ap = a->a_next;
					attr_free( a );
					break;
				}
			}
		}
	}

	slap_op_time( &op->o_time, &op->o_tincr );
	switch ( syncstate ) {
	case LDAP_SYNC_ADD:
	case LDAP_SYNC_MODIFY:
		{
			Attribute *a = attr_find( entry->e_attrs, slap_schema.si_ad_entryCSN );
			if ( a )
				op->o_csn = a->a_vals[0];
		}
retry_add:;
		if ( BER_BVISNULL( &dni.dn )) {

			op->o_req_dn = entry->e_name;
			op->o_req_ndn = entry->e_nname;
			op->o_tag = LDAP_REQ_ADD;
			op->ora_e = entry;

			rc = be->be_add( op, &rs_add );
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: rid %03d be_add (%d)\n", 
					si->si_rid, rc, 0 );
			switch ( rs_add.sr_err ) {
			case LDAP_SUCCESS:
				be_entry_release_w( op, entry );
				ret = 0;
				break;

			case LDAP_REFERRAL:
			/* we assume that LDAP_NO_SUCH_OBJECT is returned 
			 * only if the suffix entry is not present */
			case LDAP_NO_SUCH_OBJECT:
				syncrepl_add_glue( op, entry );
				ret = 0;
				break;

			/* if an entry was added via syncrepl_add_glue(),
			 * it likely has no entryUUID, so the previous
			 * be_search() doesn't find it.  In this case,
			 * give syncrepl a chance to modify it. Also
			 * allow for entries that were recreated with the
			 * same DN but a different entryUUID.
			 */
			case LDAP_ALREADY_EXISTS:
				if ( retry ) {
					Operation	op2 = *op;
					SlapReply	rs2 = { 0 };
					slap_callback	cb2 = { 0 };

					op2.o_tag = LDAP_REQ_SEARCH;
					op2.o_req_dn = entry->e_name;
					op2.o_req_ndn = entry->e_nname;
					op2.ors_scope = LDAP_SCOPE_BASE;
					op2.ors_deref = LDAP_DEREF_NEVER;
					op2.ors_attrs = slap_anlist_all_attributes;
					op2.ors_attrsonly = 0;
					op2.ors_limit = NULL;
					op2.ors_slimit = 1;
					op2.ors_tlimit = SLAP_NO_LIMIT;

					f.f_choice = LDAP_FILTER_PRESENT;
					f.f_desc = slap_schema.si_ad_objectClass;
					op2.ors_filter = &f;
					op2.ors_filterstr = generic_filterstr;

					op2.o_callback = &cb2;
					cb2.sc_response = dn_callback;
					cb2.sc_private = &dni;

					be->be_search( &op2, &rs2 );

					retry = 0;
					slap_op_time( &op->o_time, &op->o_tincr );
					goto retry_add;
				}
				/* FALLTHRU */

			default:
				Debug( LDAP_DEBUG_ANY,
					"syncrepl_entry: rid %03d be_add failed (%d)\n",
					si->si_rid, rs_add.sr_err, 0 );
				ret = 1;
				break;
			}
			goto done;
		}
		/* FALLTHRU */
		op->o_req_dn = dni.dn;
		op->o_req_ndn = dni.ndn;
		if ( dni.renamed ) {
			struct berval noldp, newp, nnewp;

			op->o_tag = LDAP_REQ_MODRDN;
			dnRdn( &entry->e_name, &op->orr_newrdn );
			dnRdn( &entry->e_nname, &op->orr_nnewrdn );

			dnParent( &dni.ndn, &noldp );
			dnParent( &entry->e_nname, &nnewp );
			if ( !dn_match( &noldp, &nnewp )) {
				dnParent( &entry->e_name, &newp );
				op->orr_newSup = &newp;
				op->orr_nnewSup = &nnewp;
			} else {
				op->orr_newSup = NULL;
				op->orr_nnewSup = NULL;
			}
			op->orr_deleteoldrdn = 0;
			rc = be->be_modrdn( op, &rs_modify );
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: rid %03d be_modrdn (%d)\n", 
					si->si_rid, rc, 0 );
			if ( rs_modify.sr_err == LDAP_SUCCESS ) {
				op->o_req_dn = entry->e_name;
				op->o_req_ndn = entry->e_nname;
			} else {
				ret = 1;
				goto done;
			}
			if ( dni.wasChanged )
				slap_op_time( &op->o_time, &op->o_tincr );
		}
		if ( dni.wasChanged ) {
			Modifications *mod, *modhead = NULL;
			Modifications *modtail = NULL;
			int i;

			op->o_tag = LDAP_REQ_MODIFY;

			assert( *modlist != NULL );

			/* Delete all the old attrs */
			for ( i = 0; i < dni.attrs; i++ ) {
				mod = ch_malloc( sizeof( Modifications ) );
				mod->sml_op = LDAP_MOD_DELETE;
				mod->sml_flags = 0;
				mod->sml_desc = dni.ads[i];
				mod->sml_type = mod->sml_desc->ad_cname;
				mod->sml_values = NULL;
				mod->sml_nvalues = NULL;
				if ( !modhead ) modhead = mod;
				if ( modtail ) {
					modtail->sml_next = mod;
				}
				modtail = mod;
			}

			/* Append passed in list to ours */
			if ( modtail ) {
				modtail->sml_next = *modlist;
				*modlist = modhead;
			} else {
				mod = *modlist;
			}

			/* Find end of this list */
			for ( ; mod != NULL; mod = mod->sml_next ) {
				modtail = mod;
			}

			mod = (Modifications *)ch_calloc(1, sizeof(Modifications));
			mod->sml_op = LDAP_MOD_REPLACE;
			mod->sml_flags = 0;
			mod->sml_desc = slap_schema.si_ad_entryUUID;
			mod->sml_type = mod->sml_desc->ad_cname;
			ber_dupbv( &uuid_bv, &syncUUID_strrep );
			ber_bvarray_add( &mod->sml_values, &uuid_bv );
			ber_dupbv( &uuid_bv, syncUUID );
			ber_bvarray_add( &mod->sml_nvalues, &uuid_bv );
			modtail->sml_next = mod;
					
			op->o_tag = LDAP_REQ_MODIFY;
			op->orm_modlist = *modlist;

			rc = be->be_modify( op, &rs_modify );
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: rid %03d be_modify (%d)\n", 
					si->si_rid, rc, 0 );
			if ( rs_modify.sr_err != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
					"syncrepl_entry: rid %03d be_modify failed (%d)\n",
					si->si_rid, rs_modify.sr_err, 0 );
			}
		}
		ret = 1;
		goto done;
	case LDAP_SYNC_DELETE :
		if ( !BER_BVISNULL( &dni.dn )) {
			op->o_req_dn = dni.dn;
			op->o_req_ndn = dni.ndn;
			op->o_tag = LDAP_REQ_DELETE;
			rc = be->be_delete( op, &rs_delete );
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: rid %03d be_delete (%d)\n", 
					si->si_rid, rc, 0 );

			while ( rs_delete.sr_err == LDAP_SUCCESS
				&& op->o_delete_glue_parent ) {
				op->o_delete_glue_parent = 0;
				if ( !be_issuffix( op->o_bd, &op->o_req_ndn )) {
					slap_callback cb = { NULL };
					cb.sc_response = slap_null_cb;
					dnParent( &op->o_req_ndn, &pdn );
					op->o_req_dn = pdn;
					op->o_req_ndn = pdn;
					op->o_callback = &cb;
					op->o_bd->be_delete( op, &rs_delete );
				} else {
					break;
				}
			}
		}
		ret = 0;
		goto done;

	default :
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_entry: rid %03d unknown syncstate\n", si->si_rid, 0, 0 );
		ret = 1;
		goto done;
	}

done :
	if ( !BER_BVISNULL( &syncUUID_strrep ) ) {
		slap_sl_free( syncUUID_strrep.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &syncUUID_strrep );
	}
	if ( dni.ads ) {
		op->o_tmpfree( dni.ads, op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &dni.ndn ) ) {
		op->o_tmpfree( dni.ndn.bv_val, op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &dni.dn ) ) {
		op->o_tmpfree( dni.dn.bv_val, op->o_tmpmemctx );
	}
	BER_BVZERO( &op->o_csn );
	return ret;
}

static struct berval gcbva[] = {
	BER_BVC("top"),
	BER_BVC("glue"),
	BER_BVNULL
};

#define NP_DELETE_ONE	2

static void
syncrepl_del_nonpresent(
	Operation *op,
	syncinfo_t *si,
	BerVarray uuids,
	struct berval *cookiecsn )
{
	Backend* be = op->o_bd;
	slap_callback	cb = { NULL };
	SlapReply	rs_search = {REP_RESULT};
	SlapReply	rs_delete = {REP_RESULT};
	SlapReply	rs_modify = {REP_RESULT};
	struct nonpresent_entry *np_list, *np_prev;
	int rc;
	AttributeName	an[2];

	struct berval pdn = BER_BVNULL;
	struct berval csn;

	op->o_req_dn = si->si_base;
	op->o_req_ndn = si->si_base;

	cb.sc_response = nonpresent_callback;
	cb.sc_private = si;

	op->o_callback = &cb;
	op->o_tag = LDAP_REQ_SEARCH;
	op->ors_scope = si->si_scope;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->o_time = slap_get_time();
	op->ors_tlimit = SLAP_NO_LIMIT;


	if ( uuids ) {
		Filter uf;
#ifdef LDAP_COMP_MATCH
		AttributeAssertion eq = { NULL, BER_BVNULL, NULL };
#else
		AttributeAssertion eq = { NULL, BER_BVNULL };
#endif
		int i;

		op->ors_attrsonly = 1;
		op->ors_attrs = slap_anlist_no_attrs;
		op->ors_limit = NULL;
		op->ors_filter = &uf;

		uf.f_ava = &eq;
		uf.f_av_desc = slap_schema.si_ad_entryUUID;
		uf.f_next = NULL;
		uf.f_choice = LDAP_FILTER_EQUALITY;
		si->si_refreshDelete |= NP_DELETE_ONE;

		for (i=0; uuids[i].bv_val; i++) {
			op->ors_slimit = 1;
			slap_uuidstr_from_normalized( &uf.f_av_value, &uuids[i],
				op->o_tmpmemctx );
			filter2bv_x( op, op->ors_filter, &op->ors_filterstr );
			op->o_tmpfree( uf.f_av_value.bv_val, op->o_tmpmemctx );
			uf.f_av_value = uuids[i];
			rc = be->be_search( op, &rs_search );
			op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
		}
		si->si_refreshDelete ^= NP_DELETE_ONE;
	} else {
		memset( &an[0], 0, 2 * sizeof( AttributeName ) );
		an[0].an_name = slap_schema.si_ad_entryUUID->ad_cname;
		an[0].an_desc = slap_schema.si_ad_entryUUID;
		op->ors_attrs = an;
		op->ors_slimit = SLAP_NO_LIMIT;
		op->ors_attrsonly = 0;
		op->ors_filter = str2filter_x( op, si->si_filterstr.bv_val );
		op->ors_filterstr = si->si_filterstr;
		op->o_nocaching = 1;

		if ( limits_check( op, &rs_search ) == 0 ) {
			rc = be->be_search( op, &rs_search );
		}
		if ( op->ors_filter ) filter_free_x( op, op->ors_filter );
	}

	op->o_nocaching = 0;

	if ( !LDAP_LIST_EMPTY( &si->si_nonpresentlist ) ) {

		if ( cookiecsn && !BER_BVISNULL( cookiecsn ))
			csn = *cookiecsn;
		else
			csn = si->si_syncCookie.ctxcsn;

		slap_queue_csn( op, &csn );

		np_list = LDAP_LIST_FIRST( &si->si_nonpresentlist );
		while ( np_list != NULL ) {
			LDAP_LIST_REMOVE( np_list, npe_link );
			np_prev = np_list;
			np_list = LDAP_LIST_NEXT( np_list, npe_link );
			op->o_tag = LDAP_REQ_DELETE;
			op->o_callback = &cb;
			cb.sc_response = null_callback;
			cb.sc_private = si;
			op->o_req_dn = *np_prev->npe_name;
			op->o_req_ndn = *np_prev->npe_nname;
			rc = op->o_bd->be_delete( op, &rs_delete );
			Debug( LDAP_DEBUG_SYNC,
				"syncrepl_del_nonpresent: rid %03d be_delete %s (%d)\n", 
				si->si_rid, op->o_req_dn.bv_val, rc );

			if ( rs_delete.sr_err == LDAP_NOT_ALLOWED_ON_NONLEAF ) {
				Modifications mod1, mod2;
				mod1.sml_op = LDAP_MOD_REPLACE;
				mod1.sml_flags = 0;
				mod1.sml_desc = slap_schema.si_ad_objectClass;
				mod1.sml_type = mod1.sml_desc->ad_cname;
				mod1.sml_values = &gcbva[0];
				mod1.sml_nvalues = NULL;
				mod1.sml_next = &mod2;

				mod2.sml_op = LDAP_MOD_REPLACE;
				mod2.sml_flags = 0;
				mod2.sml_desc = slap_schema.si_ad_structuralObjectClass;
				mod2.sml_type = mod2.sml_desc->ad_cname;
				mod2.sml_values = &gcbva[1];
				mod2.sml_nvalues = NULL;
				mod2.sml_next = NULL;

				op->o_tag = LDAP_REQ_MODIFY;
				op->orm_modlist = &mod1;

				rc = be->be_modify( op, &rs_modify );
				if ( mod2.sml_next ) slap_mods_free( mod2.sml_next, 1 );
			}

			while ( rs_delete.sr_err == LDAP_SUCCESS &&
					op->o_delete_glue_parent ) {
				op->o_delete_glue_parent = 0;
				if ( !be_issuffix( op->o_bd, &op->o_req_ndn )) {
					slap_callback cb = { NULL };
					cb.sc_response = slap_null_cb;
					dnParent( &op->o_req_ndn, &pdn );
					op->o_req_dn = pdn;
					op->o_req_ndn = pdn;
					op->o_callback = &cb;
					/* give it a root privil ? */
					op->o_bd->be_delete( op, &rs_delete );
				} else {
					break;
			    }
			}

			op->o_delete_glue_parent = 0;

			ber_bvfree( np_prev->npe_name );
			ber_bvfree( np_prev->npe_nname );
			ch_free( np_prev );
		}

		slap_graduate_commit_csn( op );
		op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &op->o_csn );
	}

	return;
}

void
syncrepl_add_glue(
	Operation* op,
	Entry *e )
{
	Backend *be = op->o_bd;
	slap_callback cb = { NULL };
	Attribute	*a;
	int	rc;
	int suffrdns;
	int i;
	struct berval dn = BER_BVNULL;
	struct berval ndn = BER_BVNULL;
	Entry	*glue;
	SlapReply	rs_add = {REP_RESULT};
	struct berval	ptr, nptr;
	char		*comma;

	op->o_tag = LDAP_REQ_ADD;
	op->o_callback = &cb;
	cb.sc_response = null_callback;
	cb.sc_private = NULL;

	dn = e->e_name;
	ndn = e->e_nname;

	/* count RDNs in suffix */
	if ( !BER_BVISEMPTY( &be->be_nsuffix[0] ) ) {
		for ( i = 0, ptr = be->be_nsuffix[0], comma = ptr.bv_val; comma != NULL; comma = ber_bvchr( &ptr, ',' ) ) {
			comma++;
			ptr.bv_len -= comma - ptr.bv_val;
			ptr.bv_val = comma;
			i++;
		}
		suffrdns = i;
	} else {
		/* suffix is "" */
		suffrdns = 0;
	}

	/* Start with BE suffix */
	ptr = dn;
	for ( i = 0; i < suffrdns; i++ ) {
		comma = ber_bvrchr( &ptr, ',' );
		if ( comma != NULL ) {
			ptr.bv_len = comma - ptr.bv_val;
		} else {
			ptr.bv_len = 0;
			break;
		}
	}
	
	if ( !BER_BVISEMPTY( &ptr ) ) {
		dn.bv_len -= ptr.bv_len + 1;
		dn.bv_val += ptr.bv_len + 1;
	}

	/* the normalizedDNs are always the same length, no counting
	 * required.
	 */
	nptr = ndn;
	if ( ndn.bv_len > be->be_nsuffix[0].bv_len ) {
		ndn.bv_val += ndn.bv_len - be->be_nsuffix[0].bv_len;
		ndn.bv_len = be->be_nsuffix[0].bv_len;

		nptr.bv_len = ndn.bv_val - nptr.bv_val - 1;

	} else {
		nptr.bv_len = 0;
	}

	while ( ndn.bv_val > e->e_nname.bv_val ) {
		glue = (Entry *) ch_calloc( 1, sizeof(Entry) );
		ber_dupbv( &glue->e_name, &dn );
		ber_dupbv( &glue->e_nname, &ndn );

		a = ch_calloc( 1, sizeof( Attribute ));
		a->a_desc = slap_schema.si_ad_objectClass;

		a->a_vals = ch_calloc( 3, sizeof( struct berval ));
		ber_dupbv( &a->a_vals[0], &gcbva[0] );
		ber_dupbv( &a->a_vals[1], &gcbva[1] );
		ber_dupbv( &a->a_vals[2], &gcbva[2] );

		a->a_nvals = a->a_vals;

		a->a_next = glue->e_attrs;
		glue->e_attrs = a;

		a = ch_calloc( 1, sizeof( Attribute ));
		a->a_desc = slap_schema.si_ad_structuralObjectClass;

		a->a_vals = ch_calloc( 2, sizeof( struct berval ));
		ber_dupbv( &a->a_vals[0], &gcbva[1] );
		ber_dupbv( &a->a_vals[1], &gcbva[2] );

		a->a_nvals = a->a_vals;

		a->a_next = glue->e_attrs;
		glue->e_attrs = a;

		op->o_req_dn = glue->e_name;
		op->o_req_ndn = glue->e_nname;
		op->ora_e = glue;
		rc = be->be_add ( op, &rs_add );
		if ( rs_add.sr_err == LDAP_SUCCESS ) {
			be_entry_release_w( op, glue );
		} else {
		/* incl. ALREADY EXIST */
			entry_free( glue );
		}

		/* Move to next child */
		comma = ber_bvrchr( &ptr, ',' );
		if ( comma == NULL ) {
			break;
		}
		ptr.bv_len = comma - ptr.bv_val;
		
		dn.bv_val = ++comma;
		dn.bv_len = e->e_name.bv_len - (dn.bv_val - e->e_name.bv_val);

		comma = ber_bvrchr( &nptr, ',' );
		assert( comma != NULL );
		nptr.bv_len = comma - nptr.bv_val;

		ndn.bv_val = ++comma;
		ndn.bv_len = e->e_nname.bv_len - (ndn.bv_val - e->e_nname.bv_val);
	}

	op->o_req_dn = e->e_name;
	op->o_req_ndn = e->e_nname;
	op->ora_e = e;
	rc = be->be_add ( op, &rs_add );
	if ( rs_add.sr_err == LDAP_SUCCESS ) {
		be_entry_release_w( op, e );
	} else {
		entry_free( e );
	}

	return;
}

static void
syncrepl_updateCookie(
	syncinfo_t *si,
	Operation *op,
	struct berval *pdn,
	struct sync_cookie *syncCookie )
{
	Backend *be = op->o_bd;
	Modifications mod = { { 0 } };
	struct berval vals[ 2 ];

	int rc;

	slap_callback cb = { NULL };
	SlapReply	rs_modify = {REP_RESULT};

	slap_sync_cookie_free( &si->si_syncCookie, 0 );
	slap_dup_sync_cookie( &si->si_syncCookie, syncCookie );

	mod.sml_op = LDAP_MOD_REPLACE;
	mod.sml_desc = slap_schema.si_ad_contextCSN;
	mod.sml_type = mod.sml_desc->ad_cname;
	mod.sml_values = vals;
	vals[0] = si->si_syncCookie.ctxcsn;
	vals[1].bv_val = NULL;
	vals[1].bv_len = 0;

	slap_queue_csn( op, &si->si_syncCookie.ctxcsn );

	op->o_tag = LDAP_REQ_MODIFY;

	assert( si->si_rid < 1000 );

	cb.sc_response = null_callback;
	cb.sc_private = si;

	op->o_callback = &cb;
	op->o_req_dn = op->o_bd->be_suffix[0];
	op->o_req_ndn = op->o_bd->be_nsuffix[0];

	/* update contextCSN */
	op->o_msgid = SLAP_SYNC_UPDATE_MSGID;
	op->orm_modlist = &mod;
	rc = be->be_modify( op, &rs_modify );
	op->o_msgid = 0;

	if ( rs_modify.sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_updateCookie: rid %03d be_modify failed (%d)\n",
			si->si_rid, rs_modify.sr_err, 0 );
	}

	slap_graduate_commit_csn( op );
	op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
	BER_BVZERO( &op->o_csn );
	if ( mod.sml_next ) slap_mods_free( mod.sml_next, 1 );

	return;
}

static int
dn_callback(
	Operation*	op,
	SlapReply*	rs )
{
	dninfo *dni = op->o_callback->sc_private;

	if ( rs->sr_type == REP_SEARCH ) {
		if ( !BER_BVISNULL( &dni->dn ) ) {
			Debug( LDAP_DEBUG_ANY,
				"dn_callback: consistency error - "
				"entryUUID is not unique\n", 0, 0, 0 );
		} else {
			ber_dupbv_x( &dni->dn, &rs->sr_entry->e_name, op->o_tmpmemctx );
			ber_dupbv_x( &dni->ndn, &rs->sr_entry->e_nname, op->o_tmpmemctx );
			/* If there is a new entry, see if it differs from the old.
			 * We compare the non-normalized values so that cosmetic changes
			 * in the provider are always propagated.
			 */
			if ( dni->new_entry ) {
				Attribute *old, *new;
				int i;

				/* Did the DN change? Note that we don't explicitly try to
				 * discover if the deleteOldRdn argument applies here. It
				 * would save an unnecessary Modify if we detected it, but
				 * that's a fair amount of trouble to compare the two attr
				 * lists in detail. (Just test normalized DN; we ignore
				 * insignificant changes here.)
				 */
				if ( !dn_match( &rs->sr_entry->e_nname,
						&dni->new_entry->e_nname ) )
				{
					dni->renamed = 1;
				}

				for ( i = 0, old = rs->sr_entry->e_attrs;
						old;
						i++, old = old->a_next )
					;

				dni->attrs = i;

				/* We assume that attributes are saved in the same order
				 * in the remote and local databases. So if we walk through
				 * the attributeDescriptions one by one they should match in
				 * lock step. If not, we signal a change. Otherwise we test
				 * all the values...
				 */
				for ( old = rs->sr_entry->e_attrs, new = dni->new_entry->e_attrs;
						old && new;
						old = old->a_next, new = new->a_next )
				{
					if ( old->a_desc != new->a_desc ) {
						dni->wasChanged = 1;
						break;
					}
					for ( i = 0; ; i++ ) {
						int nold, nnew;
						nold = BER_BVISNULL( &old->a_vals[i] );
						nnew = BER_BVISNULL( &new->a_vals[i] );
						/* If both are empty, stop looking */
						if ( nold && nnew ) {
							break;
						}
						/* If they are different, stop looking */
						if ( nold != nnew ) {
							dni->wasChanged = 1;
							break;
						}
						if ( ber_bvcmp( &old->a_vals[i], &new->a_vals[i] )) {
							dni->wasChanged = 1;
							break;
						}
					}
					if ( dni->wasChanged ) break;
				}
				if ( dni->wasChanged ) {
					dni->ads = op->o_tmpalloc( dni->attrs *
						sizeof(AttributeDescription *), op->o_tmpmemctx );
					i = 0;
					for ( old = rs->sr_entry->e_attrs; old; old = old->a_next ) {
						dni->ads[i] = old->a_desc;
						i++;
					}
				}
			}
		}
	} else if ( rs->sr_type == REP_RESULT ) {
		if ( rs->sr_err == LDAP_SIZELIMIT_EXCEEDED ) {
			Debug( LDAP_DEBUG_ANY,
				"dn_callback: consistency error - "
				"entryUUID is not unique\n", 0, 0, 0 );
		}
	}

	return LDAP_SUCCESS;
}

static int
nonpresent_callback(
	Operation*	op,
	SlapReply*	rs )
{
	syncinfo_t *si = op->o_callback->sc_private;
	Attribute *a;
	int count = 0;
	struct berval* present_uuid = NULL;
	struct nonpresent_entry *np_entry;

	if ( rs->sr_type == REP_RESULT ) {
		count = avl_free( si->si_presentlist, avl_ber_bvfree );
		si->si_presentlist = NULL;

	} else if ( rs->sr_type == REP_SEARCH ) {
		if ( !(si->si_refreshDelete & NP_DELETE_ONE )) {
			a = attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_entryUUID );

			if ( a == NULL ) return 0;

			present_uuid = avl_find( si->si_presentlist, &a->a_nvals[0],
				syncuuid_cmp );
		}

		if ( present_uuid == NULL ) {
			np_entry = (struct nonpresent_entry *)
				ch_calloc( 1, sizeof( struct nonpresent_entry ));
			np_entry->npe_name = ber_dupbv( NULL, &rs->sr_entry->e_name );
			np_entry->npe_nname = ber_dupbv( NULL, &rs->sr_entry->e_nname );
			LDAP_LIST_INSERT_HEAD( &si->si_nonpresentlist, np_entry, npe_link );

		} else {
			avl_delete( &si->si_presentlist,
					&a->a_nvals[0], syncuuid_cmp );
			ch_free( present_uuid->bv_val );
			ch_free( present_uuid );
		}
	}
	return LDAP_SUCCESS;
}

static int
null_callback(
	Operation*	op,
	SlapReply*	rs )
{
	if ( rs->sr_err != LDAP_SUCCESS &&
		rs->sr_err != LDAP_REFERRAL &&
		rs->sr_err != LDAP_ALREADY_EXISTS &&
		rs->sr_err != LDAP_NO_SUCH_OBJECT &&
		rs->sr_err != LDAP_NOT_ALLOWED_ON_NONLEAF )
	{
		Debug( LDAP_DEBUG_ANY,
			"null_callback: error code 0x%x\n",
			rs->sr_err, 0, 0 );
	}
	return LDAP_SUCCESS;
}

static struct berval *
slap_uuidstr_from_normalized(
	struct berval* uuidstr,
	struct berval* normalized,
	void *ctx )
{
	struct berval *new;
	unsigned char nibble;
	int i, d = 0;

	if ( normalized == NULL ) return NULL;
	if ( normalized->bv_len != 16 ) return NULL;

	if ( uuidstr ) {
		new = uuidstr;
	} else {
		new = (struct berval *)slap_sl_malloc( sizeof(struct berval), ctx );
		if ( new == NULL ) {
			return NULL;
		}
	}

	new->bv_len = 36;

	if ( ( new->bv_val = slap_sl_malloc( new->bv_len + 1, ctx ) ) == NULL ) {
		if ( new != uuidstr ) {
			slap_sl_free( new, ctx );
		}
		return NULL;
	}

	for ( i = 0; i < 16; i++ ) {
		if ( i == 4 || i == 6 || i == 8 || i == 10 ) {
			new->bv_val[(i<<1)+d] = '-';
			d += 1;
		}

		nibble = (normalized->bv_val[i] >> 4) & 0xF;
		if ( nibble < 10 ) {
			new->bv_val[(i<<1)+d] = nibble + '0';
		} else {
			new->bv_val[(i<<1)+d] = nibble - 10 + 'a';
		}

		nibble = (normalized->bv_val[i]) & 0xF;
		if ( nibble < 10 ) {
			new->bv_val[(i<<1)+d+1] = nibble + '0';
		} else {
			new->bv_val[(i<<1)+d+1] = nibble - 10 + 'a';
		}
	}

	new->bv_val[new->bv_len] = '\0';
	return new;
}

static int
syncuuid_cmp( const void* v_uuid1, const void* v_uuid2 )
{
	const struct berval *uuid1 = v_uuid1;
	const struct berval *uuid2 = v_uuid2;
	int rc = uuid1->bv_len - uuid2->bv_len;
	if ( rc ) return rc;
	return ( memcmp( uuid1->bv_val, uuid2->bv_val, uuid1->bv_len ) );
}

static void
avl_ber_bvfree( void *v_bv )
{
	struct berval	*bv = (struct berval *)v_bv;
	
	if( v_bv == NULL ) return;
	if ( !BER_BVISNULL( bv ) ) {
		ch_free( bv->bv_val );
	}
	ch_free( (char *) bv );
}

void
syncinfo_free( syncinfo_t *sie )
{
 	if ( sie->si_ld ) {
 		if ( sie->si_conn_setup ) {
 			ber_socket_t s;
 			ldap_get_option( sie->si_ld, LDAP_OPT_DESC, &s );
 			connection_client_stop( s );
 			sie->si_conn_setup = 0;
 		}
 		ldap_unbind_ext( sie->si_ld, NULL, NULL );
 	}

 	ldap_pvt_thread_mutex_destroy( &sie->si_mutex );

	bindconf_free( &sie->si_bindconf );

	if ( sie->si_filterstr.bv_val ) {
		ch_free( sie->si_filterstr.bv_val );
	}
	if ( sie->si_base.bv_val ) {
		ch_free( sie->si_base.bv_val );
	}
	if ( sie->si_attrs ) {
		int i = 0;
		while ( sie->si_attrs[i] != NULL ) {
			ch_free( sie->si_attrs[i] );
			i++;
		}
		ch_free( sie->si_attrs );
	}
	if ( sie->si_exattrs ) {
		int i = 0;
		while ( sie->si_exattrs[i] != NULL ) {
			ch_free( sie->si_exattrs[i] );
			i++;
		}
		ch_free( sie->si_exattrs );
	}
	if ( sie->si_anlist ) {
		int i = 0;
		while ( sie->si_anlist[i].an_name.bv_val != NULL ) {
			ch_free( sie->si_anlist[i].an_name.bv_val );
			i++;
		}
		ch_free( sie->si_anlist );
	}
	if ( sie->si_exanlist ) {
		int i = 0;
		while ( sie->si_exanlist[i].an_name.bv_val != NULL ) {
			ch_free( sie->si_exanlist[i].an_name.bv_val );
			i++;
		}
		ch_free( sie->si_exanlist );
	}
	if ( sie->si_retryinterval ) {
		ch_free( sie->si_retryinterval );
	}
	if ( sie->si_retrynum ) {
		ch_free( sie->si_retrynum );
	}
	if ( sie->si_retrynum_init ) {
		ch_free( sie->si_retrynum_init );
	}
	slap_sync_cookie_free( &sie->si_syncCookie, 0 );
	if ( sie->si_presentlist ) {
	    avl_free( sie->si_presentlist, avl_ber_bvfree );
	}
	while ( !LDAP_LIST_EMPTY( &sie->si_nonpresentlist )) {
		struct nonpresent_entry* npe;
		npe = LDAP_LIST_FIRST( &sie->si_nonpresentlist );
		LDAP_LIST_REMOVE( npe, npe_link );
		if ( npe->npe_name ) {
			if ( npe->npe_name->bv_val ) {
				ch_free( npe->npe_name->bv_val );
			}
			ch_free( npe->npe_name );
		}
		if ( npe->npe_nname ) {
			if ( npe->npe_nname->bv_val ) {
				ch_free( npe->npe_nname->bv_val );
			}
			ch_free( npe->npe_nname );
		}
		ch_free( npe );
	}
	ch_free( sie );
}



/* NOTE: used & documented in slapd.conf(5) */
#define IDSTR			"rid"
#define PROVIDERSTR		"provider"
#define SCHEMASTR		"schemachecking"
#define FILTERSTR		"filter"
#define SEARCHBASESTR		"searchbase"
#define SCOPESTR		"scope"
#define ATTRSONLYSTR		"attrsonly"
#define ATTRSSTR		"attrs"
#define TYPESTR			"type"
#define INTERVALSTR		"interval"
#define RETRYSTR		"retry"
#define SLIMITSTR		"sizelimit"
#define TLIMITSTR		"timelimit"
#define SYNCDATASTR		"syncdata"

/* FIXME: undocumented */
#define	LOGBASESTR	"logbase"
#define LOGFILTERSTR	"logfilter"
#define OLDAUTHCSTR		"bindprincipal"
#define EXATTRSSTR		"exattrs"
#define MANAGEDSAITSTR		"manageDSAit"

/* FIXME: unused */
#define LASTMODSTR		"lastmod"
#define LMGENSTR		"gen"
#define LMNOSTR			"no"
#define LMREQSTR		"req"
#define SRVTABSTR		"srvtab"
#define SUFFIXSTR		"suffix"

/* mandatory */
#define GOT_ID			0x0001
#define GOT_PROVIDER		0x0002
#define GOT_BASE		0x0004

/* check */
#define GOT_ALL			(GOT_ID|GOT_PROVIDER|GOT_BASE)

static struct {
	struct berval key;
	int val;
} scopes[] = {
	{ BER_BVC("base"), LDAP_SCOPE_BASE },
	{ BER_BVC("one"), LDAP_SCOPE_ONELEVEL },
	{ BER_BVC("onelevel"), LDAP_SCOPE_ONELEVEL },	/* OpenLDAP extension */
	{ BER_BVC("children"), LDAP_SCOPE_SUBORDINATE },
	{ BER_BVC("subordinate"), LDAP_SCOPE_SUBORDINATE },
	{ BER_BVC("sub"), LDAP_SCOPE_SUBTREE },
	{ BER_BVC("subtree"), LDAP_SCOPE_SUBTREE },	/* OpenLDAP extension */
	{ BER_BVNULL, 0 }
};

static slap_verbmasks datamodes[] = {
	{ BER_BVC("default"), SYNCDATA_DEFAULT },
	{ BER_BVC("accesslog"), SYNCDATA_ACCESSLOG },
	{ BER_BVC("changelog"), SYNCDATA_CHANGELOG },
	{ BER_BVNULL, 0 }
};

static int
parse_syncrepl_line(
	ConfigArgs	*c,
	syncinfo_t	*si )
{
	int	gots = 0;
	int	i;
	char	*val;

	for ( i = 1; i < c->argc; i++ ) {
		if ( !strncasecmp( c->argv[ i ], IDSTR "=",
					STRLENOF( IDSTR "=" ) ) )
		{
			int tmp;
			/* '\0' string terminator accounts for '=' */
			val = c->argv[ i ] + STRLENOF( IDSTR "=" );
			if ( lutil_atoi( &tmp, val ) != 0 ) {
				snprintf( c->msg, sizeof( c->msg ),
					"Error: parse_syncrepl_line: "
					"unable to parse syncrepl id \"%s\"", val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return -1;
			}
			if ( tmp >= 1000 || tmp < 0 ) {
				snprintf( c->msg, sizeof( c->msg ),
					"Error: parse_syncrepl_line: "
					"syncrepl id %d is out of range [0..999]", tmp );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return -1;
			}
			si->si_rid = tmp;
			gots |= GOT_ID;
		} else if ( !strncasecmp( c->argv[ i ], PROVIDERSTR "=",
					STRLENOF( PROVIDERSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( PROVIDERSTR "=" );
			ber_str2bv( val, 0, 1, &si->si_bindconf.sb_uri );
			gots |= GOT_PROVIDER;
		} else if ( !strncasecmp( c->argv[ i ], SCHEMASTR "=",
					STRLENOF( SCHEMASTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( SCHEMASTR "=" );
			if ( !strncasecmp( val, "on", STRLENOF( "on" ) )) {
				si->si_schemachecking = 1;
			} else if ( !strncasecmp( val, "off", STRLENOF( "off" ) ) ) {
				si->si_schemachecking = 0;
			} else {
				si->si_schemachecking = 1;
			}
		} else if ( !strncasecmp( c->argv[ i ], FILTERSTR "=",
					STRLENOF( FILTERSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( FILTERSTR "=" );
			if ( si->si_filterstr.bv_val )
				ch_free( si->si_filterstr.bv_val );
			ber_str2bv( val, 0, 1, &si->si_filterstr );
		} else if ( !strncasecmp( c->argv[ i ], LOGFILTERSTR "=",
					STRLENOF( LOGFILTERSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( LOGFILTERSTR "=" );
			if ( si->si_logfilterstr.bv_val )
				ch_free( si->si_logfilterstr.bv_val );
			ber_str2bv( val, 0, 1, &si->si_logfilterstr );
		} else if ( !strncasecmp( c->argv[ i ], SEARCHBASESTR "=",
					STRLENOF( SEARCHBASESTR "=" ) ) )
		{
			struct berval	bv;
			int		rc;

			val = c->argv[ i ] + STRLENOF( SEARCHBASESTR "=" );
			if ( si->si_base.bv_val ) {
				ch_free( si->si_base.bv_val );
			}
			ber_str2bv( val, 0, 0, &bv );
			rc = dnNormalize( 0, NULL, NULL, &bv, &si->si_base, NULL );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->msg, sizeof( c->msg ),
					"Invalid base DN \"%s\": %d (%s)",
					val, rc, ldap_err2string( rc ) );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return -1;
			}
			gots |= GOT_BASE;
		} else if ( !strncasecmp( c->argv[ i ], LOGBASESTR "=",
					STRLENOF( LOGBASESTR "=" ) ) )
		{
			struct berval	bv;
			int		rc;

			val = c->argv[ i ] + STRLENOF( LOGBASESTR "=" );
			if ( si->si_logbase.bv_val ) {
				ch_free( si->si_logbase.bv_val );
			}
			ber_str2bv( val, 0, 0, &bv );
			rc = dnNormalize( 0, NULL, NULL, &bv, &si->si_logbase, NULL );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->msg, sizeof( c->msg ),
					"Invalid logbase DN \"%s\": %d (%s)",
					val, rc, ldap_err2string( rc ) );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return -1;
			}
		} else if ( !strncasecmp( c->argv[ i ], SCOPESTR "=",
					STRLENOF( SCOPESTR "=" ) ) )
		{
			int j;
			val = c->argv[ i ] + STRLENOF( SCOPESTR "=" );
			for ( j=0; !BER_BVISNULL(&scopes[j].key); j++ ) {
				if (!strcasecmp( val, scopes[j].key.bv_val )) {
					si->si_scope = scopes[j].val;
					break;
				}
			}
			if ( BER_BVISNULL(&scopes[j].key) ) {
				snprintf( c->msg, sizeof( c->msg ),
					"Error: parse_syncrepl_line: "
					"unknown scope \"%s\"", val);
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return -1;
			}
		} else if ( !strncasecmp( c->argv[ i ], ATTRSONLYSTR,
					STRLENOF( ATTRSONLYSTR ) ) )
		{
			si->si_attrsonly = 1;
		} else if ( !strncasecmp( c->argv[ i ], ATTRSSTR "=",
					STRLENOF( ATTRSSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( ATTRSSTR "=" );
			if ( !strncasecmp( val, ":include:", STRLENOF(":include:") ) ) {
				char *attr_fname;
				attr_fname = ch_strdup( val + STRLENOF(":include:") );
				si->si_anlist = file2anlist( si->si_anlist, attr_fname, " ,\t" );
				if ( si->si_anlist == NULL ) {
					ch_free( attr_fname );
					return -1;
				}
				si->si_anfile = attr_fname;
			} else {
				char *str, *s, *next;
				char delimstr[] = " ,\t";
				str = ch_strdup( val );
				for ( s = ldap_pvt_strtok( str, delimstr, &next );
						s != NULL;
						s = ldap_pvt_strtok( NULL, delimstr, &next ) )
				{
					if ( strlen(s) == 1 && *s == '*' ) {
						si->si_allattrs = 1;
						*(val + ( s - str )) = delimstr[0];
					}
					if ( strlen(s) == 1 && *s == '+' ) {
						si->si_allopattrs = 1;
						*(val + ( s - str )) = delimstr[0];
					}
				}
				ch_free( str );
				si->si_anlist = str2anlist( si->si_anlist, val, " ,\t" );
				if ( si->si_anlist == NULL ) {
					return -1;
				}
			}
		} else if ( !strncasecmp( c->argv[ i ], EXATTRSSTR "=",
					STRLENOF( EXATTRSSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( EXATTRSSTR "=" );
			if ( !strncasecmp( val, ":include:", STRLENOF(":include:") )) {
				char *attr_fname;
				attr_fname = ch_strdup( val + STRLENOF(":include:") );
				si->si_exanlist = file2anlist(
					si->si_exanlist, attr_fname, " ,\t" );
				if ( si->si_exanlist == NULL ) {
					ch_free( attr_fname );
					return -1;
				}
				ch_free( attr_fname );
			} else {
				si->si_exanlist = str2anlist( si->si_exanlist, val, " ,\t" );
				if ( si->si_exanlist == NULL ) {
					return -1;
				}
			}
		} else if ( !strncasecmp( c->argv[ i ], TYPESTR "=",
					STRLENOF( TYPESTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( TYPESTR "=" );
			if ( !strncasecmp( val, "refreshOnly",
						STRLENOF("refreshOnly") ) )
			{
				si->si_type = si->si_ctype = LDAP_SYNC_REFRESH_ONLY;
			} else if ( !strncasecmp( val, "refreshAndPersist",
						STRLENOF("refreshAndPersist") ) )
			{
				si->si_type = si->si_ctype = LDAP_SYNC_REFRESH_AND_PERSIST;
				si->si_interval = 60;
			} else {
				snprintf( c->msg, sizeof( c->msg ),
					"Error: parse_syncrepl_line: "
					"unknown sync type \"%s\"", val);
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return -1;
			}
		} else if ( !strncasecmp( c->argv[ i ], INTERVALSTR "=",
					STRLENOF( INTERVALSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( INTERVALSTR "=" );
			if ( si->si_type == LDAP_SYNC_REFRESH_AND_PERSIST ) {
				si->si_interval = 0;
			} else if ( strchr( val, ':' ) != NULL ) {
				char *next, *ptr = val;
				unsigned dd, hh, mm, ss;
				
				/* NOTE: the test for ptr[ 0 ] == '-'
				 * should go before the call to strtoul() */
				dd = strtoul( ptr, &next, 10 );
				if ( ptr[ 0 ] == '-' || next == ptr || next[0] != ':' ) {
					snprintf( c->msg, sizeof( c->msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse days", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
					return -1;
				}
				ptr = next + 1;
				hh = strtoul( ptr, &next, 10 );
				if ( ptr[ 0 ] == '-' || next == ptr || next[0] != ':' || hh > 24 ) {
					snprintf( c->msg, sizeof( c->msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse hours", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
					return -1;
				}
				ptr = next + 1;
				mm = strtoul( ptr, &next, 10 );
				if ( ptr[ 0 ] == '-' || next == ptr || next[0] != ':' || mm > 60 ) {
					snprintf( c->msg, sizeof( c->msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse minutes", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
					return -1;
				}
				ptr = next + 1;
				ss = strtoul( ptr, &next, 10 );
				if ( ptr[ 0 ] == '-' || next == ptr || next[0] != '\0' || ss > 60 ) {
					snprintf( c->msg, sizeof( c->msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse seconds", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
					return -1;
				}
				si->si_interval = (( dd * 24 + hh ) * 60 + mm ) * 60 + ss;
			} else {
				unsigned long	t;

				if ( lutil_parse_time( val, &t ) != 0 ) {
					snprintf( c->msg, sizeof( c->msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\"", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
					return -1;
				}
				si->si_interval = (time_t)t;
			}
			if ( si->si_interval < 0 ) {
				snprintf( c->msg, sizeof( c->msg ),
					"Error: parse_syncrepl_line: "
					"invalid interval \"%ld\"",
					(long) si->si_interval);
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return -1;
			}
		} else if ( !strncasecmp( c->argv[ i ], RETRYSTR "=",
					STRLENOF( RETRYSTR "=" ) ) )
		{
			char **retry_list;
			int j, k, n;

			val = c->argv[ i ] + STRLENOF( RETRYSTR "=" );
			retry_list = (char **) ch_calloc( 1, sizeof( char * ));
			retry_list[0] = NULL;

			slap_str2clist( &retry_list, val, " ,\t" );

			for ( k = 0; retry_list && retry_list[k]; k++ ) ;
			n = k / 2;
			if ( k % 2 ) {
				snprintf( c->msg, sizeof( c->msg ),
					"Error: incomplete syncrepl retry list" );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				for ( k = 0; retry_list && retry_list[k]; k++ ) {
					ch_free( retry_list[k] );
				}
				ch_free( retry_list );
				return 1;
			}
			si->si_retryinterval = (time_t *) ch_calloc( n + 1, sizeof( time_t ));
			si->si_retrynum = (int *) ch_calloc( n + 1, sizeof( int ));
			si->si_retrynum_init = (int *) ch_calloc( n + 1, sizeof( int ));
			for ( j = 0; j < n; j++ ) {
				unsigned long	t;
				if ( lutil_atoul( &t, retry_list[j*2] ) != 0 ) {
					snprintf( c->msg, sizeof( c->msg ),
						"Error: invalid retry interval \"%s\" (#%d)",
						retry_list[j*2], j );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
					/* do some cleanup */
					return 1;
				}
				si->si_retryinterval[j] = (time_t)t;
				if ( *retry_list[j*2+1] == '+' ) {
					si->si_retrynum_init[j] = RETRYNUM_FOREVER;
					si->si_retrynum[j] = RETRYNUM_FOREVER;
					j++;
					break;
				} else {
					if ( lutil_atoi( &si->si_retrynum_init[j], retry_list[j*2+1] ) != 0
							|| si->si_retrynum_init[j] <= 0 )
					{
						snprintf( c->msg, sizeof( c->msg ),
							"Error: invalid initial retry number \"%s\" (#%d)",
							retry_list[j*2+1], j );
						Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
						/* do some cleanup */
						return 1;
					}
					if ( lutil_atoi( &si->si_retrynum[j], retry_list[j*2+1] ) != 0
							|| si->si_retrynum[j] <= 0 )
					{
						snprintf( c->msg, sizeof( c->msg ),
							"Error: invalid retry number \"%s\" (#%d)",
							retry_list[j*2+1], j );
						Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
						/* do some cleanup */
						return 1;
					}
				}
			}
			si->si_retrynum_init[j] = RETRYNUM_TAIL;
			si->si_retrynum[j] = RETRYNUM_TAIL;
			si->si_retryinterval[j] = 0;
			
			for ( k = 0; retry_list && retry_list[k]; k++ ) {
				ch_free( retry_list[k] );
			}
			ch_free( retry_list );
		} else if ( !strncasecmp( c->argv[ i ], MANAGEDSAITSTR "=",
					STRLENOF( MANAGEDSAITSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( MANAGEDSAITSTR "=" );
			if ( lutil_atoi( &si->si_manageDSAit, val ) != 0
				|| si->si_manageDSAit < 0 || si->si_manageDSAit > 1 )
			{
				snprintf( c->msg, sizeof( c->msg ),
					"invalid manageDSAit value \"%s\".\n",
					val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return 1;
			}
		} else if ( !strncasecmp( c->argv[ i ], SLIMITSTR "=",
					STRLENOF( SLIMITSTR "=") ) )
		{
			val = c->argv[ i ] + STRLENOF( SLIMITSTR "=" );
			if ( strcasecmp( val, "unlimited" ) == 0 ) {
				si->si_slimit = 0;

			} else if ( lutil_atoi( &si->si_slimit, val ) != 0 || si->si_slimit < 0 ) {
				snprintf( c->msg, sizeof( c->msg ),
					"invalid size limit value \"%s\".\n",
					val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return 1;
			}
		} else if ( !strncasecmp( c->argv[ i ], TLIMITSTR "=",
					STRLENOF( TLIMITSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( TLIMITSTR "=" );
			if ( strcasecmp( val, "unlimited" ) == 0 ) {
				si->si_tlimit = 0;

			} else if ( lutil_atoi( &si->si_tlimit, val ) != 0 || si->si_tlimit < 0 ) {
				snprintf( c->msg, sizeof( c->msg ),
					"invalid time limit value \"%s\".\n",
					val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
				return 1;
			}
		} else if ( !strncasecmp( c->argv[ i ], SYNCDATASTR "=",
					STRLENOF( SYNCDATASTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( SYNCDATASTR "=" );
			si->si_syncdata = verb_to_mask( val, datamodes );
		} else if ( bindconf_parse( c->argv[i], &si->si_bindconf ) ) {
			snprintf( c->msg, sizeof( c->msg ),
				"Error: parse_syncrepl_line: "
				"unable to parse \"%s\"\n", c->argv[ i ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
			return -1;
		}
	}

	if ( gots != GOT_ALL ) {
		snprintf( c->msg, sizeof( c->msg ),
			"Error: Malformed \"syncrepl\" line in slapd config file, missing%s%s%s",
			gots & GOT_ID ? "" : " "IDSTR,
			gots & GOT_PROVIDER ? "" : " "PROVIDERSTR,
			gots & GOT_BASE ? "" : " "SEARCHBASESTR );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->msg, 0 );
		return -1;
	}

	return 0;
}

static int
add_syncrepl(
	ConfigArgs *c )
{
	syncinfo_t *si;
	int	rc = 0;

	if ( !( c->be->be_search && c->be->be_add && c->be->be_modify && c->be->be_delete ) ) {
		snprintf( c->msg, sizeof(c->msg), "database %s does not support "
			"operations required for syncrepl", c->be->be_type );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->msg, 0 );
		return 1;
	}
	if ( BER_BVISEMPTY( &c->be->be_rootdn )) {
		strcpy( c->msg, "rootDN must be defined before syncrepl may be used" );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->msg, 0 );
		return 1;
	}
	si = (syncinfo_t *) ch_calloc( 1, sizeof( syncinfo_t ) );

	if ( si == NULL ) {
		Debug( LDAP_DEBUG_ANY, "out of memory in add_syncrepl\n", 0, 0, 0 );
		return 1;
	}

	si->si_bindconf.sb_tls = SB_TLS_OFF;
	si->si_bindconf.sb_method = LDAP_AUTH_SIMPLE;
	si->si_schemachecking = 0;
	ber_str2bv( "(objectclass=*)", STRLENOF("(objectclass=*)"), 1,
		&si->si_filterstr );
	si->si_base.bv_val = NULL;
	si->si_scope = LDAP_SCOPE_SUBTREE;
	si->si_attrsonly = 0;
	si->si_anlist = (AttributeName *) ch_calloc( 1, sizeof( AttributeName ));
	si->si_exanlist = (AttributeName *) ch_calloc( 1, sizeof( AttributeName ));
	si->si_attrs = NULL;
	si->si_allattrs = 0;
	si->si_allopattrs = 0;
	si->si_exattrs = NULL;
	si->si_type = si->si_ctype = LDAP_SYNC_REFRESH_ONLY;
	si->si_interval = 86400;
	si->si_retryinterval = NULL;
	si->si_retrynum_init = NULL;
	si->si_retrynum = NULL;
	si->si_manageDSAit = 0;
	si->si_tlimit = 0;
	si->si_slimit = 0;
	si->si_conn_setup = 0;

	si->si_presentlist = NULL;
	LDAP_LIST_INIT( &si->si_nonpresentlist );
	ldap_pvt_thread_mutex_init( &si->si_mutex );

	rc = parse_syncrepl_line( c, si );

	if ( rc == 0 ) {
		si->si_be = c->be;
		init_syncrepl( si );
		si->si_re = ldap_pvt_runqueue_insert( &slapd_rq, si->si_interval,
			do_syncrepl, si, "do_syncrepl", c->be->be_suffix[0].bv_val );
		if ( !si->si_re )
			rc = -1;
	}
	if ( rc < 0 ) {
		Debug( LDAP_DEBUG_ANY, "failed to add syncinfo\n", 0, 0, 0 );
		syncinfo_free( si );	
		return 1;
	} else {
		Debug( LDAP_DEBUG_CONFIG,
			"Config: ** successfully added syncrepl \"%s\"\n",
			BER_BVISNULL( &si->si_bindconf.sb_uri ) ?
			"(null)" : si->si_bindconf.sb_uri.bv_val, 0, 0 );
		if ( !si->si_schemachecking ) {
			SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_NO_SCHEMA_CHECK;
		}
		c->be->be_syncinfo = si;
		return 0;
	}
}

static void
syncrepl_unparse( syncinfo_t *si, struct berval *bv )
{
	struct berval bc, uri;
	char buf[BUFSIZ*2], *ptr;
	int i;

	/* FIXME: we're not checking for buf[] overflow! */

	/* temporarily inhibit bindconf from printing URI */
	uri = si->si_bindconf.sb_uri;
	BER_BVZERO( &si->si_bindconf.sb_uri );
	bindconf_unparse( &si->si_bindconf, &bc );
	si->si_bindconf.sb_uri = uri;

	ptr = buf;
	ptr += snprintf( ptr, sizeof( buf ), IDSTR "=%03ld " PROVIDERSTR "=%s",
		si->si_rid, si->si_bindconf.sb_uri.bv_val );
	if ( !BER_BVISNULL( &bc )) {
		ptr = lutil_strcopy( ptr, bc.bv_val );
		free( bc.bv_val );
	}
	if ( !BER_BVISEMPTY( &si->si_filterstr )) {
		ptr = lutil_strcopy( ptr, " " FILTERSTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_filterstr.bv_val );
		*ptr++ = '"';
	}
	if ( !BER_BVISNULL( &si->si_base )) {
		ptr = lutil_strcopy( ptr, " " SEARCHBASESTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_base.bv_val );
		*ptr++ = '"';
	}
	if ( !BER_BVISEMPTY( &si->si_logfilterstr )) {
		ptr = lutil_strcopy( ptr, " " LOGFILTERSTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_logfilterstr.bv_val );
		*ptr++ = '"';
	}
	if ( !BER_BVISNULL( &si->si_logbase )) {
		ptr = lutil_strcopy( ptr, " " LOGBASESTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_logbase.bv_val );
		*ptr++ = '"';
	}
	for (i=0; !BER_BVISNULL(&scopes[i].key);i++) {
		if ( si->si_scope == scopes[i].val ) {
			ptr = lutil_strcopy( ptr, " " SCOPESTR "=" );
			ptr = lutil_strcopy( ptr, scopes[i].key.bv_val );
			break;
		}
	}
	if ( si->si_attrsonly ) {
		ptr = lutil_strcopy( ptr, " " ATTRSONLYSTR );
	}
	if ( si->si_anfile ) {
		ptr = lutil_strcopy( ptr, " " ATTRSSTR "=:include:" );
		ptr = lutil_strcopy( ptr, si->si_anfile );
	} else if ( si->si_allattrs || si->si_allopattrs ||
		( si->si_anlist && !BER_BVISNULL(&si->si_anlist[0].an_name) )) {
		char *old;
		ptr = lutil_strcopy( ptr, " " ATTRSSTR "=\"" );
		old = ptr;
		ptr = anlist_unparse( si->si_anlist, ptr );
		if ( si->si_allattrs ) {
			if ( old != ptr ) *ptr++ = ',';
			*ptr++ = '*';
		}
		if ( si->si_allopattrs ) {
			if ( old != ptr ) *ptr++ = ',';
			*ptr++ = '+';
		}
		*ptr++ = '"';
	}
	if ( si->si_exanlist && !BER_BVISNULL(&si->si_exanlist[0].an_name) ) {
		ptr = lutil_strcopy( ptr, " " EXATTRSSTR "=" );
		ptr = anlist_unparse( si->si_exanlist, ptr );
	}
	ptr = lutil_strcopy( ptr, " " SCHEMASTR "=" );
	ptr = lutil_strcopy( ptr, si->si_schemachecking ? "on" : "off" );
	
	ptr = lutil_strcopy( ptr, " " TYPESTR "=" );
	ptr = lutil_strcopy( ptr, si->si_type == LDAP_SYNC_REFRESH_AND_PERSIST ?
		"refreshAndPersist" : "refreshOnly" );

	if ( si->si_type == LDAP_SYNC_REFRESH_ONLY ) {
		int dd, hh, mm, ss;

		dd = si->si_interval;
		ss = dd % 60;
		dd /= 60;
		mm = dd % 60;
		dd /= 60;
		hh = dd % 24;
		dd /= 24;
		ptr = lutil_strcopy( ptr, " " INTERVALSTR "=" );
		ptr += sprintf( ptr, "%02d:%02d:%02d:%02d", dd, hh, mm, ss );
	} else if ( si->si_retryinterval ) {
		int space=0;
		ptr = lutil_strcopy( ptr, " " RETRYSTR "=\"" );
		for (i=0; si->si_retryinterval[i]; i++) {
			if ( space ) *ptr++ = ' ';
			space = 1;
			ptr += sprintf( ptr, "%ld ", (long) si->si_retryinterval[i] );
			if ( si->si_retrynum_init[i] == RETRYNUM_FOREVER )
				*ptr++ = '+';
			else
				ptr += sprintf( ptr, "%d", si->si_retrynum_init[i] );
		}
		*ptr++ = '"';
	}

	if ( si->si_slimit ) {
		ptr = lutil_strcopy( ptr, " " SLIMITSTR "=" );
		ptr += sprintf( ptr, "%d", si->si_slimit );
	}

	if ( si->si_tlimit ) {
		ptr = lutil_strcopy( ptr, " " TLIMITSTR "=" );
		ptr += sprintf( ptr, "%d", si->si_tlimit );
	}

	if ( si->si_syncdata ) {
		if ( enum_to_verb( datamodes, si->si_syncdata, &bc ) >= 0 ) {
			ptr = lutil_strcopy( ptr, " " SYNCDATASTR "=" );
			ptr = lutil_strcopy( ptr, bc.bv_val );
		}
	}
	bc.bv_len = ptr - buf;
	bc.bv_val = buf;
	ber_dupbv( bv, &bc );
}

int
syncrepl_config( ConfigArgs *c )
{
	if (c->op == SLAP_CONFIG_EMIT) {
		if ( c->be->be_syncinfo ) {
			struct berval bv;
			syncrepl_unparse( c->be->be_syncinfo, &bv ); 
			ber_bvarray_add( &c->rvalue_vals, &bv );
			return 0;
		}
		return 1;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		struct re_s *re;

		if ( c->be->be_syncinfo ) {
			re = c->be->be_syncinfo->si_re;
			if ( re ) {
				if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ) )
					ldap_pvt_runqueue_stoptask( &slapd_rq, re );
				ldap_pvt_runqueue_remove( &slapd_rq, re );
			}
			syncinfo_free( c->be->be_syncinfo );
			c->be->be_syncinfo = NULL;
		}
		SLAP_DBFLAGS( c->be ) &= ~(SLAP_DBFLAG_SHADOW|SLAP_DBFLAG_SYNC_SHADOW);
		return 0;
	}
	if ( SLAP_SHADOW( c->be ) ) {
		Debug(LDAP_DEBUG_ANY, "%s: "
			"syncrepl: database already shadowed.\n",
			c->log, 0, 0);
		return(1);
	} else if ( add_syncrepl( c ) ) {
		return(1);
	}
	return config_sync_shadow( c );
}
