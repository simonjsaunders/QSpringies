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

#ifndef STATE_H
#define STATE_H

#define BF_NUM    4
#define DEF_TSTEP 0.025

/* Forces */
#define FR_GRAV      0
#define FR_CMASS     1
#define FR_PTATTRACT 2
#define FR_WALL      3

struct State {
    State();
    void reset();
    double cur_mass, cur_rest;
    double cur_ks, cur_kd;
    bool fix_mass, show_spring;
    int center_id;
    bool force_enabled[BF_NUM];
    double cur_grav_val[BF_NUM];
    double cur_misc_val[BF_NUM];
    double cur_visc, cur_stick;
    double cur_dt, cur_prec;
    bool adaptive_step, grid_snap;
    double cur_gsnap;
    bool w_top, w_left, w_right, w_bottom;
    bool collide;
};

#endif
