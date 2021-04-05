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

#ifndef MISC_H
#define MISC_H

template <typename NumericType>
NumericType deltaX(NumericType dx) { return dx; }

template <typename NumericType>
NumericType deltaY(NumericType dy) { return -dy; }

#define NAIL_SIZE	4

int massRadius(double);
int sphereSize(int);
int sphereRadius(int);

inline int screenRadius(int radius) {
    return sphereRadius(sphereSize(radius));
}

inline double square(double x) { return x * x; }

#endif // MISC_H
