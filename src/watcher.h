// $Id$

/*****************************************************************************
 * Copyright 2005 - 2011 Peter Berck                                                *
 *                                                                           *
 * This file is part of PETeR.                                          *
 *                                                                           *
 * PETeR is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * PETeR is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with PETeR; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ----------------------------------------------------------------------------

#ifndef _watcher_h
#define _watcher_h

#include <string>
#include <vector>

#include "Config.h"

// C includes
//
#include <sys/time.h>
#include <unistd.h>

// ----------------------------------------------------------------------------

struct wopr_msgbuf {
  long mtype;
  int verbose;
  int foo;
};

extern "C" void* watcher( void * );
void create_watcher( Config&  );
#endif
