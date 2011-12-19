// $Id: $
//

/*****************************************************************************
 * Copyright 2008 - 2011 Peter Berck                                         *
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

#ifndef _PREDEDIT_H
#define _PREDEDIT_H

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

int pdt( Logfile&, Config& );
int pdt2( Logfile&, Config& );
//void generate_next(  Timbl::TimblAPI*, Config&, std::string, int, std::vector<distr_elem>& );
int window_letters( Logfile&, Config& );

#endif
