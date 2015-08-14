/* $OpenLDAP: pkg/ldap/libraries/librewrite/rewrite-map.h,v 1.5.2.3 2007/01/02 21:43:52 kurt Exp $ */
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
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Pierangelo Masarati for
 * inclusion in OpenLDAP Software.
 */

#ifndef MAP_H
#define MAP_H

/*
 * Retrieves a builtin map
 */
LDAP_REWRITE_F (struct rewrite_builtin_map *)
rewrite_builtin_map_find(
                struct rewrite_info *info,
                const char *name
);


/*
 * LDAP map
 */
LDAP_REWRITE_F (void  *)
map_ldap_parse(
		struct rewrite_info *info,
		const char *fname,
		int lineno,
	       	int argc,
	       	char **argv
);

LDAP_REWRITE_F (int)
map_ldap_apply( struct rewrite_builtin_map *map,
		const char *filter,
		struct berval *val
);

LDAP_REWRITE_F (int)
map_ldap_destroy( struct rewrite_builtin_map **map );

#endif /* MAP_H */
