/*
 * Copyright (c) 2020 Simon J. Saunders
 *
 * This file is part of QSpringies.
 *
 * QSpringies is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QSpringies is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QSpringies.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "misc.h"
#include <math.h>

int massRadius(double m)
{
    int rad = (int)(2 * log(4.0 * m + 1.0));

    if (rad < 1)
        rad = 1;
    if (rad > 64)
        rad = 64;

    // TODO spthick
    return rad /*+ spthick/2*/;
}

int sphereSize(int rad)
{
    rad = (25 + (2 * rad))/2;
    if (rad < 15)
        rad = 15;
    int size = 0;
    if (rad * 2 >= 30)
        size = ((rad * 2) - 30) / 10;
    if (size > 4)
        size = 4;
    return size;
}

int sphereRadius(int size)
{
    return (size*10 + 30)/2;
}
