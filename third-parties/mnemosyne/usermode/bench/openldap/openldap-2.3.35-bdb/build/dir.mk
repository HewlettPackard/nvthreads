# $OpenLDAP: pkg/ldap/build/dir.mk,v 1.14.2.4 2007/01/02 21:43:40 kurt Exp $
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2007 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##---------------------------------------------------------------------------
#
# Makes subdirectories
#


all-common: FORCE
	@echo "Making all in `$(PWD)`"
	@for i in $(SUBDIRS) $(ALLDIRS); do 		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) all );		\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

install-common: FORCE
	@echo "Making install in `$(PWD)`"
	@for i in $(SUBDIRS) $(INSTALLDIRS); do 	\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) install );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

clean-common: FORCE
	@echo "Making clean in `$(PWD)`"
	@for i in $(SUBDIRS) $(CLEANDIRS); do		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) clean );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

veryclean-common: FORCE
	@echo "Making veryclean in `$(PWD)`"
	@for i in $(SUBDIRS) $(CLEANDIRS); do		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) veryclean );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

depend-common: FORCE
	@echo "Making depend in `$(PWD)`"
	@for i in $(SUBDIRS) $(DEPENDDIRS); do		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) depend );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

Makefile: $(top_srcdir)/build/dir.mk
