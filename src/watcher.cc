
// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2009 Peter Berck                                         *
 *                                                                           *
 * This file is part of wopr.                                                *
 *                                                                           *
 * wopr is free software; you can redistribute it and/or modify it           *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * wopr is distributed in the hope that it will be useful, but WITHOUT       *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with wpred; if not, write to the Free Software Foundation,          *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <sys/msg.h>

#include "qlog.h"
#include "Config.h"
#include "util.h"
#include "watcher.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

extern "C" void* watcher( void *params ) {
  Config *c = (Config*)params;

  // Init message queue.
  //
  key_t wopr_id = (key_t)stoi( c->get_value( "id", "88" ));
  int   msqid   = msgget( wopr_id, 0666 | IPC_CREAT );
  struct wopr_msgbuf wmb = {2, 0, 0}; // verbosity, stop

  while (1) {
    // std::cout << c->get_status() << "\n";
    // Should check msg queue, set status to 0 to quit.

    // Check if we got a message on our queue.
    //
    int bytes = msgrcv( msqid, &wmb, sizeof(wmb), 2, 0 );
    if ( bytes > 0 ) {
      if ( wmb.foo == 0 ) {
	c->set_status(0);
      }
    }
    
  }
}

void create_watcher( Config& c ) {
  pthread_attr_t attr;
  pthread_t      thread_id;
  pthread_attr_init( &attr );
  pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
  int res = pthread_create( &thread_id, &attr, &watcher, &c );
  if ( res != 0 ) {
    //
  }
  pthread_attr_destroy( &attr );
}


// ---------------------------------------------------------------------------
