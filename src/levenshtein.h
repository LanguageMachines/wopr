// $Id: levenshtein.h 2000 2008-03-19 14:16:19Z pberck $
//

/*****************************************************************************
 * Copyright 2008 Peter Berck                                                *
 *                                                                           *
 * This file is part of wopr.                                                *
 *                                                                           *
 * wpred is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * wpred is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with wpred; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

#ifndef _LEVENSHTEIN_H
#define _LEVENSHTEIN_H

int distance(const std::string, const std::string);
int levenshtein( Logfile&, Config& );
int correct( Logfile&, Config& );

#endif
