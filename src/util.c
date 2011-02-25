/* 
 * This file is part of Luna
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/******************************************************************************
 *
 *  Utilities (util.c)
 *  ---
 *  Helper functions
 *  
 *  Created: 25.02.2011 16:15:05
 *
 ******************************************************************************/
#include <stdio.h>

#include "util.h"


char *itoa(int n)
{
    static char numstr[21];

    snprintf(numstr, 21, "%d", n);

    return numstr;
}

