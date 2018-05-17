/****************************************************************************
 * Main developer, C# developing:                                           *
 * Copyright (C) 2014-2016 by Sergey Zheigurov                              *
 * Russia, Novy Urengoy                                                     *
 * zheigurov@gmail.com                                                      *
 *                                                                          *
 * C# to Qt portation, Linux developing                                     *
 * Copyright (C) 2015-2018 by Eduard Kalinowski                             *
 * Germany, Lower Saxony, Hanover                                           *
 * eduard_kalinowski@yahoo.de                                               *
 *                                                                          *
 * C# project CNC-controller-for-mk1                                        *
 * https://github.com/selenur/CNC-controller-for-mk1                        *
 *                                                                          *
 * The Qt project                                                           *
 * https://github.com/eduard-x/cnc-qt                                       *
 *                                                                          *
 * CNC-Qt is free software; may be distributed and/or modified under the    *
 * terms of the GNU General Public License version 3 as published by the    *
 * Free Software Foundation and appearing in the file LICENSE_GPLv3         *
 * included in the packaging of this file.                                  *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU Lesser General Public         *
 * License along with CNC-Qt. If not, see  http://www.gnu.org/licenses      *
 ****************************************************************************/

#include "includes/GData.h"


/**
 * @brief consructor
 *
 */
ParserData::ParserData()
{
    numberLine = 0;
    decoded = true;

    gCmd = -1;
    gExtCmd = -1;
    mCmd = -1;
    mExtCmd = -1;

    coord = QVector3D(0.0, 0.0, 0.0); // X, Y, Z

    paramName = 0x0;
    paramValue = 0.0;

    useExtCoord = NoEXT;

    extCoord = { 0.0, 0.0, 0.0 };

    lineComment = "";

    labelNum = -1;
};


/**
 * @brief constructor based on command
 *
 */
ParserData::ParserData(ParserData *d)
{
    gCmd = -1;
    gExtCmd = -1;
    mCmd = -1;
    mExtCmd = -1;

    coord = d->coord;
    useExtCoord = NoEXT;

    extCoord = { 0.0, 0.0, 0.0 }; // for ABC, IJK, UVW

    decoded = true;

    // for M commands
    paramName = 0x0;
    paramValue = 0.0;

    lineComment = "";

    labelNum = -1;

    numberLine = d->numberLine;
};

