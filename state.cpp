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

#include "state.h"

State::State()
{
    reset();
}

void State::reset()
{
    cur_mass = 1.0;
    cur_rest = 1.0;
    cur_ks = 1.0;
    cur_kd = 1.0;
    fix_mass = false;
    show_spring = true;
    center_id = -1;
    for (int i = 0; i < BF_NUM; ++i)
        force_enabled[i] = false;
    cur_grav_val[FR_GRAV] = 10.0;
    cur_grav_val[FR_CMASS] = 5.0;
    cur_grav_val[FR_PTATTRACT] = 10.0;
    cur_grav_val[FR_WALL] = 10000.0;
    cur_misc_val[FR_GRAV] = 0.0;
    cur_misc_val[FR_CMASS] = 2.0;
    cur_misc_val[FR_PTATTRACT] = 0.0;
    cur_misc_val[FR_WALL] = 1.0;
    cur_visc = 0.0;
    cur_stick = 0.0;
    cur_dt = DEF_TSTEP;
    cur_prec = 1.0;
    adaptive_step = false;
    grid_snap = false;
    cur_gsnap = 20.0;
    w_top = true;
    w_left = true;
    w_right = true;
    w_bottom = true;
    collide = false;
}
