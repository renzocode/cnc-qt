/****************************************************************************
 * Main developer, C# developing:                                           *
 * Copyright (C) 2014-2016 by Sergey Zheigurov                              *
 * Russia, Novy Urengoy                                                     *
 * zheigurov@gmail.com                                                      *
 *                                                                          *
 * C# to Qt portation, Linux developing                                     *
 * Copyright (C) 2015-2016 by Eduard Kalinowski                             *
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


#include <QObject>
#include <QRegExp>
#include <QDebug>
#include <QString>

#include <cmath>
#include <limits>

#include "includes/Settings.h"
#include "includes/GCode.h"
#include "includes/MainWindow.h"



GCodeData::GCodeData()
{
    changeInstrument = false;
    numberInstrument = 0;
    //  needPause        = false;
    pauseMSeconds      = -1;

    X = 0.0;
    Y = 0.0;
    Z = 0.0;
    A = 0.0;

    // arc parameters
    I = 0.0;
    J = 0.0;
    K = 0.0;

    plane = NonePlane;
    changeDirection = false;

    Radius = 0.0;
    vectorCoeff = 0.0;
    // end of arc

    typeMoving = NoType;

    accelCode =  NO_CODE;

    vectSpeed = 0.0;

    stepsCounter = 0;

    angle = 0.0;

    spindelON = false;
    splits = 0; // init
    numberLine = 0;
    feed      = false;
    diametr = 0.0;
};


// constructor based on existing command
GCodeData::GCodeData(GCodeData *d)
{
    X = d->X;
    Y = d->Y;
    Z = d->Z;
    A = d->A;

    I = d->I;
    J = d->J;
    K = d->K;

    Radius = d->Radius;

    plane = d->plane;

    changeDirection = false;
    vectorCoeff = 0.0;

    typeMoving = d->typeMoving;

    spindelON = d->spindelON;
    vectSpeed = d->vectSpeed;

    splits = 0; // if arc, will be splitted
    stepsCounter = 0; // should calculated

    accelCode = d->accelCode;
    //  numberInstruct = d->numberInstruct;
    numberLine = d->numberLine;
    feed = d->feed;

    angle = 0.0;//d->angleVectors;

    changeInstrument = d->changeInstrument;
    numberInstrument = d->numberInstrument;
    pauseMSeconds = d->pauseMSeconds;
    diametr = d->diametr;
};



GCodeParser::GCodeParser() // constructor
{

}


bool GCodeParser::addLine(GCodeData *c)
{
}


bool GCodeParser::addArc(GCodeData *c)
{
}


// read and parse into GCodeData list and OpenGL list
bool GCodeParser::readGCode(const QByteArray &gcode)
{
    //  QMutexLocker mLock(&mutex);
    gCodeList.clear();

    //  lock();

    //     cached_lines.clear();
    //     cached_points.clear();
    //     cached_color.clear();

    goodList.clear();
    badList.clear();

    //  unlock();

    QTextStream stream(gcode);
    stream.setLocale(QLocale("C"));
    // or this ? QString.split(QRegExp("\n|\r\n|\r"));
    // if we switch the input methode from QTextStream to QStringList, performance is about 15% higher

    Vec3 origin(0, 0, 0);
    Vec3 current_pos(0, 0, 0);
    bool b_absolute = true;
    float coef = 1.0; // 1 or 24.5

    QTime t;
    t.start();

    bool decoded;
    int index = 0;
    QStringList gCodeLines;
    QString lastCmd;

    while(!stream.atEnd()) { // restruct lines
        QString lineStream = stream.readLine().toUpper().trimmed();

        if (lineStream.isEmpty() || lineStream.at(0) == ';' || lineStream.at(0) == '(' || lineStream.at(0) == '%') {
            continue;
        }

        int posComment = lineStream.indexOf(";");

        if (posComment >= 0) {
            lineStream = lineStream.mid(posComment);

            if (lineStream.isEmpty()) {
                continue;
            }
        }

        int commentBeg = lineStream.indexOf('(');
        int commentEnd = -1;

        if (commentBeg >= 0) {
            commentEnd = lineStream.lastIndexOf(')');

            if (commentEnd > commentBeg) {
                lineStream = lineStream.remove(commentBeg, commentEnd - commentBeg + 1);
            }
        }

        //      while (lineStream.length() > 0 && commentBeg >= 0 && commentEnd >= 0) {
        //  lineStream = lineStream.remove(commentBeg, commentEnd - commentBeg + 1);
        //  if (lineStream.length() > 0){
        //   commentBeg = lineStream.indexOf('(');
        //   commentEnd = lineStream.lastIndexOf(')');
        //  }
        //      }

        if (lineStream.length() == 0) {
            continue;
        }

        //      lineStream = lineStream.remove(' ');

        lineStream = lineStream.replace(Settings::fromDecimalPoint, Settings::toDecimalPoint);
#if 0
        QString tmp = lineStream;

        foreach(QChar c, tmp) {
            if (c
        }
#else
        int pos = lineStream.indexOf(QRegExp("N(\\d+)"));

        if ( pos == 0) { // remove command number from lineStream
            int posNotDigit = lineStream.indexOf(QRegExp("([A-Z])"), pos + 1);

            if (posNotDigit > 0) {
                lineStream = lineStream.mid(posNotDigit);
            }
        }

        for (int iPos = 1; iPos >= 0; ) {
            iPos = lineStream.indexOf(QRegExp("([A-Z])"), iPos);

            if (iPos > 0) {
                lineStream.insert(iPos, " ");
                iPos += 2;
            }
        }

#endif

    if (lineStream.indexOf(QRegExp("[G|M|F](\\d+)($|\\s)")) == -1) { // Gxx, Fxx or Mxx not found
            if (lastCmd.length() > 0) {
                lineStream = QString(lastCmd + " " + lineStream);
            } else {
                //                 QString msg = translate(_NOT_DECODED);
                //                 badList << msg.arg(QString::number(index)) + lineStream;
                badList << QString::number(index) + ": " + lineStream;
            }
        } else {
            int posSpace = lineStream.indexOf(" ");

            if (posSpace > 0) { // command with parameter
                if (posSpace == 2) { // insert '0' if two characters
                    lineStream.insert(1, "0");
                    posSpace++;
                }

                lastCmd = lineStream.left(posSpace);

            } else { // command without parameter
                if (lineStream.length() == 2) { // insert '0' if two characters
                    lineStream.insert(1, "0");
                    posSpace++;
                }

                lastCmd = lineStream;
            }
        }

        index++;
        gCodeLines << lineStream;
    }

    qDebug("read gcode, loaded. Time elapsed: %d ms", t.elapsed());

    t.restart();

    index = 0;
    GCodeData *tmpCommand = new GCodeData();

    foreach(QString line, gCodeLines) {
        decoded = true;
        QStringList lst = line.simplified().split(" ");
        QString cmd = lst.at(0);

        if (cmd.isEmpty()) {
            continue;
        }

        bool movingCommand = true;

        switch(cmd[0].toLatin1()) {
            case 'G': {
                if (cmd == "G00") { // eilgang
                    Vec3 next_pos(b_absolute ? current_pos - origin : Vec3(0, 0, 0));
                    float E;

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    tmpCommand->X = next_pos.x();
                    tmpCommand->Y = next_pos.y();
                    tmpCommand->Z = next_pos.z();
                    tmpCommand->splits = 0;

                    tmpCommand->typeMoving = GCodeData::Line;

                    tmpCommand->feed = false;

                    if (b_absolute) {
                        current_pos = next_pos + origin;
                    } else {
                        current_pos += next_pos;
                    }

                    gCodeList << *tmpCommand;
                    // init of next instuction
                    tmpCommand = new GCodeData(tmpCommand);

                    tmpCommand->numberLine = index;

                    tmpCommand->changeInstrument = false;
                    tmpCommand->pauseMSeconds = -1; // no pause

                    break;
                }

                if (cmd == "G01") { // feed
                    Vec3 next_pos(b_absolute ? current_pos - origin : Vec3(0, 0, 0));
                    float E(-1.0);

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    tmpCommand->X = next_pos.x();
                    tmpCommand->Y = next_pos.y();
                    tmpCommand->Z = next_pos.z();
                    tmpCommand->splits = 0;

                    tmpCommand->typeMoving = GCodeData::Line;

                    tmpCommand->feed = true;

                    //                     if (E > 0.0) {
                    //                         cached_lines.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                         cached_points.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                     }

                    if (b_absolute) {
                        current_pos = next_pos + origin;
                    } else {
                        current_pos += next_pos;
                    }

                    //                     if (E > 0.0) {
                    //                         cached_lines.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                         cached_points.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                     }

                    gCodeList << *tmpCommand;
                    // init of next instuction
                    tmpCommand = new GCodeData(tmpCommand);

                    tmpCommand->numberLine = index;

                    tmpCommand->changeInstrument = false;
                    tmpCommand->pauseMSeconds = -1; // no pause

                    break;
                }

                // http://www.manufacturinget.org/2011/12/cnc-g-code-g02-and-g03/
                if (cmd == "G02" || cmd == "G03") { // arc
                    Vec3 next_pos(b_absolute ? current_pos - origin : Vec3(0, 0, 0));
                    float E(-1.0);

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    tmpCommand->plane = NonePlane;

                    tmpCommand->X = next_pos.x();
                    tmpCommand->Y = next_pos.y();
                    tmpCommand->Z = next_pos.z();

                    Vec3 arc_center(current_pos);
                    // float E_arc(-1.0);
                    float radius = 0.0;

                    if (parseArc(line, arc_center, radius, coef ) == false) {
                        decoded = false;
                        break;
                    }

                    if (radius == 0.0) {
                        // the arc center coordinateds
                        tmpCommand->I = arc_center.x();
                        tmpCommand->J = arc_center.y();
                        tmpCommand->K = arc_center.z();

                        if (tmpCommand->K == COORD_TOO_BIG) {
                            tmpCommand->plane = XY;
                        } else if (tmpCommand->I == COORD_TOO_BIG) {
                            tmpCommand->plane = YZ;
                        } else if (tmpCommand->J == COORD_TOO_BIG) {
                            tmpCommand->plane = ZX;
                        }
                    } else { // radius detected, ijk should be calculated
                        // circle ?
                        if (current_pos.x() == next_pos.x() && current_pos.y() == next_pos.y()) {
                            tmpCommand->plane = XY;
                        } else if (current_pos.y() == next_pos.y() && current_pos.z() == next_pos.z()) {
                            tmpCommand->plane = YZ;
                        } else if (current_pos.z() == next_pos.z() && current_pos.x() == next_pos.x()) {
                            tmpCommand->plane = ZX;
                        } else if((current_pos.x() != next_pos.x() || current_pos.y() != next_pos.y()) && current_pos.z() == next_pos.z()) {
                            tmpCommand->plane = XY;
                        } else if((current_pos.y() != next_pos.y() || current_pos.z() != next_pos.z()) && current_pos.x() == next_pos.x()) {
                            tmpCommand->plane = YZ;
                        } else if((current_pos.z() != next_pos.z() || current_pos.x() != next_pos.x()) && current_pos.y() == next_pos.y()) {
                            tmpCommand->plane = ZX;
                        }
                    }

                    tmpCommand->Radius = radius;

                    if (cmd == "G02" ) {
                        tmpCommand->typeMoving = GCodeData::ArcCW;
                    } else {
                        tmpCommand->typeMoving = GCodeData::ArcCCW;
                    }

                    tmpCommand->feed = true;

                    //                     if (E > 0.0) {
                    //                         //  cached_arcs.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                         cached_points.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                     }

                    if (b_absolute) {
                        current_pos = next_pos + origin;
                    } else {
                        current_pos += next_pos;
                    }

                    //                     if (E > 0.0) {
                    //                         //  cached_arcs.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                         cached_points.push_back(Vec3f(current_pos.x(), current_pos.y(), current_pos.z()));
                    //                     }

                    // qDebug() << "line " << tmpCommand->numberLine << "before convertArcToLines()" << gCodeList.count() << "splits" << tmpCommand->splits;
                    convertArcToLines(tmpCommand); // tmpCommand has data of last point
#if 0
                    gCodeList << *tmpCommand;
                    // init of next instuction
                    tmpCommand = new GCodeData(tmpCommand);
#endif
                    tmpCommand->numberLine = index;

                    tmpCommand->changeInstrument = false;
                    tmpCommand->pauseMSeconds = -1; // no pause

                    // qDebug() << "after " << gCodeList.count() << "splits" << tmpCommand->splits;
                    break;
                }

                if (cmd == "G04") {
                    // need next parameter
                    QString property1 = lst.at(1).mid(0, 1);
                    QString value1 = lst.at(1).mid(1);

                    if (property1 == "P") {
                        //  tmpCommand->needPause = true;
                        bool res;
                        tmpCommand->pauseMSeconds = value1.toInt(&res);

                        if (res == false) {
                            decoded = false;
                            break;
                        }
                    }

                    if (property1 == "X") {
                        //  tmpCommand->needPause = true;
                        bool res;
                        tmpCommand->pauseMSeconds = value1.toFloat(&res) * 1000;

                        if (res == false) {
                            decoded = false;
                            break;
                        }
                    }

                    break;
                }

                if (cmd == "G28") {
                    Vec3 next_pos(std::numeric_limits<float>::infinity(),
                                  std::numeric_limits<float>::infinity(),
                                  std::numeric_limits<float>::infinity());
                    float E;

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    if (next_pos[0] == std::numeric_limits<float>::infinity()
                            && next_pos[1] == std::numeric_limits<float>::infinity()
                            && next_pos[2] == std::numeric_limits<float>::infinity()) {
                        current_pos = origin = Vec3(0, 0, 0);
                    } else {
                        for(size_t i = 0 ; i < 3 ; ++i) {
                            if (next_pos[i] != std::numeric_limits<float>::infinity()) {
                                current_pos[i] = 0;
                                origin[i] = 0;
                            }
                        }
                    }

                    break;
                }

                if (cmd == "G20") {
                    movingCommand = false;
                    coef = 25.4;
                    break;
                }

                if (cmd == "G21") {
                    movingCommand = false;
                    coef = 1.0;
                    break;
                }

                if (cmd == "G90") {
                    movingCommand = false;
                    b_absolute = true;
                    break;
                }

                if (cmd == "G91") {
                    movingCommand = false;
                    b_absolute = false;
                    break;
                }

                if (cmd == "G92") {
                    Vec3 next_pos(current_pos);
                    float E;

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    origin = current_pos - next_pos;
                }

                break;
            }

            case 'M': {
                if (cmd == "M00") {
                    tmpCommand->pauseMSeconds = 0; // waiting
                    break;
                }

                if (cmd == "M03") {
                    tmpCommand->spindelON = true;
                    break;
                }

                if (cmd == "M05") {
                    tmpCommand->spindelON = false;
                    break;
                }

                if (cmd == "M06") {
                    // need next parameter
                    QString property1 = lst.at(1).mid(0, 1);
                    QString value1 = lst.at(1).mid(1);

                    if (property1 == "T") {
                        tmpCommand->changeInstrument = true;
                        bool res;
                        tmpCommand->numberInstrument = value1.toInt(&res);

                        if (res == false) {
                            decoded = false;
                        }

                        tmpCommand->pauseMSeconds = value1.toInt(&res);

                        if (res == false) {
                            decoded = false;
                        }

                        if (lst.count() > 2) {
                            QString property2 = lst.at(2).mid(0, 1);

                            if ( property2 == "D" ) {
                                QString value2 = lst.at(2).mid(1).replace(Settings::fromDecimalPoint, Settings::toDecimalPoint);

                                tmpCommand->diametr = value2.toDouble(&res);

                                if (res == false) {
                                    decoded = false;
                                    break;
                                }
                            }
                        }
                    }

                    break;
                }

                if (cmd == "M206") {
                    float E;

                    if (parseCoord(line, origin, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    break;
                }

                break;
            }

            case 'F': {
                decoded = false;
                break;
            }

            default:
                decoded = false;
                break;
        }

        if (decoded == false) {
            //             QString msg = translate(_NOT_DECODED);
            //             badList << msg.arg(QString::number(index)) + line;
            badList << QString::number(index) + ": " + line;
        } else {
#if 0

            if (movingCommand == true) {
                //   if (cmd != "G02" && cmd != "G03"){
                gCodeList << *tmpCommand;
                // init of next instuction

                tmpCommand = new GCodeData(tmpCommand);
                //   }


                //   tmpCommand->numberInstruct++;
                tmpCommand->numberLine = index;

                //   tmpCommand->needPause = false;
                tmpCommand->changeInstrument = false;
                tmpCommand->pauseMSeconds = -1; // no pause

            }

#endif
            goodList << line;
        }

        index++;
    }

    qDebug("read gcode, parsed. Time elapsed: %d ms", t.elapsed());
    //  qDebug() << "data parsed";
    gCodeLines.clear();

    //  delete tmpCommand;

    // qDebug() << "LIst" << goodList.count();
    //     for(size_t i = 0 ; i < cached_lines.size() ; ++i) {
    //         cached_color.push_back(Vec3f(1, 1, 1) * (float(i) / cached_lines.size()));
    //     }

    //  std::pair<Vec3, Vec3> bbox(Vec3(std::numeric_limits<float>::infinity(),
    //   std::numeric_limits<float>::infinity(),
    //   std::numeric_limits<float>::infinity()),
    // -Vec3(std::numeric_limits<float>::infinity(),
    //    std::numeric_limits<float>::infinity(),
    //    std::numeric_limits<float>::infinity()));
    //
    //  for(const auto &p : cached_points) {
    //      for(size_t i = 0 ; i < 3 ; ++i) {
    //  bbox.first[i] = std::min<float>(bbox.first[i], p[i]);
    //  bbox.second[i] = std::max<float>(bbox.second[i], p[i]);
    //      }
    //  }
    //  unlock();

    return true;
}


float GCodeParser::determineAngle(const Vec3 &pos, const Vec3 &pos_center, PlaneEnum pl)
{
    float radians = 0.0;

    switch (pl) {
        case XY: {
            if (pos[0] == pos_center[0] && pos[1] == pos_center[1]) { // if diff is 0
                return 0.0;
            }

            radians = atan2(pos[1] - pos_center[1], pos[0] - pos_center[0]);

            break;
        }

        case YZ: {
            if (pos[1] == pos_center[1] && pos[2] == pos_center[2]) {
                return 0.0;
            }

            radians = atan2(pos[2] - pos_center[2], pos[1] - pos_center[1]);

            break;
        }

        case ZX: {
            if (pos[2] == pos_center[2] && pos[0] == pos_center[0]) {
                return 0.0;
            }

            radians = atan2(pos[0] - pos_center[0], pos[2] - pos_center[2]);

            break;
        }

        default:
            qDebug() << "not defined plane of arc";
            break;
    }

    if (radians < 0.0) {
        radians += 2.0 * PI;
    }

    return radians;
}

//
// 'endData' is the pointer of arc start
void GCodeParser::convertArcToLines(GCodeData *endData)
{
    if (gCodeList.count() == 0) {
        return;
    }

    if (endData == 0) {
        return;
    }

    if (!(endData->typeMoving == GCodeData::ArcCW || endData->typeMoving == GCodeData::ArcCCW) ) { // it's not arc
        return;
    }

    GCodeData &begData = gCodeList.last();
    // arcs
    // translate points to arc
    float a, r; // length of sides
    float x2, x1, y2, y1, z2, z1;

    x1 = begData.X;;
    x2 = endData->X;

    y1 = begData.Y;
    y2 = endData->Y;

    z1 = begData.Z;
    z2 = endData->Z;

    float i, j, k;
    i = endData->I;
    j = endData->J;
    k = endData->K;

    Vec3 pos1(x1, y1, z1);
    Vec3 pos2(x2, y2, z2);

    float dPos = 0.0;
    float begPos = 0.0;

    switch (endData->plane) {
        case XY: {
            a = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));

            if (endData->Radius == 0.0) {
                r = sqrt(pow(x1 - i, 2) + pow(y1 - j, 2));
            } else {
                r = endData->Radius;
                // compute i, j
                float a = determineAngle (pos1, pos2, endData->plane) + PI;
                qDebug() << "radius " << r << "alpha" << a << "xy point 1" << x1 << y1 << "xy point 2" << x2 << y2;
            }

            dPos = z2 - z1;
            begPos = z1;
        }
        break;

        case YZ: {
            a = sqrt(pow(y2 - y1, 2) + pow(z2 - z1, 2));

            if (endData->Radius == 0.0) {
                r = sqrt(pow(y1 - j, 2) + pow(z1 - k, 2));
            } else {
                r = endData->Radius;
                // compute j, k
            }

            dPos = x2 - x1;
            begPos = x1;
        }
        break;

        case ZX: {
            a = sqrt(pow(z2 - z1, 2) + pow(x2 - x1, 2));

            if (endData->Radius == 0.0) {
                r = sqrt(pow(z1 - k, 2) + pow(x1 - i, 2));
            } else {
                r = endData->Radius;
                // compute k, i
            }

            dPos = y2 - y1;
            begPos = y1;
        }
        break;

        default:
            break;
    }

    float alpha = 0.0;
    float alpha_beg, alpha_end;

    if (r == 0.0) {
        qDebug() << "wrong, r = 0";
        return;
    }

    Vec3 posC(i, j, k);

    alpha_beg = determineAngle (pos1, posC, endData->plane);
    alpha_end = determineAngle (pos2, posC, endData->plane);

    if (endData->typeMoving == GCodeData::ArcCW) {
        if (alpha_beg == alpha_end) {
            alpha_beg += 2.0 * PI;
        }

        alpha = alpha_beg - alpha_end;

        if (alpha_beg < alpha_end) {
            alpha = fabs(alpha_beg + (2.0 * PI - alpha_end));
        }

    } else {
        if (alpha_beg == alpha_end) {
            alpha_end += 2.0 * PI;
        }

        alpha = alpha_end - alpha_beg;

        if (alpha_beg > alpha_end) {
            alpha = fabs(alpha_end + (2.0 * PI - alpha_beg));
        }
    }

    float bLength = r * alpha;

    int n = (int)(bLength * Settings::splitsPerMm) - 1; // num segments of arc per mm
    float splitLen = 1.0 / (float)Settings::splitsPerMm;

    if ( n == 0) {
        qDebug() << "wrong, n = 0" << alpha_beg << alpha_end;
        return;
    }

    float dAlpha = alpha / n;

    dPos = dPos / n;

    if (endData->typeMoving == GCodeData::ArcCW) {
        dAlpha = -dAlpha;
    }

#if DEBUG_ARC
    QString dbg;
#endif
    float angle = alpha_beg;
    float loopPos = begPos;

    // copy of parsed endpoint
    GCodeData *ncommand = new GCodeData(endData);

#if DEBUG_ARC
    qDebug() << "arc from " << begData.X << begData.Y << begData.Z  << "to" << endData->X << endData->Y << endData->Z << "splits: " << n;
#endif


    QList<GCodeData> tmpList;

    ncommand->X = begData.X;
    ncommand->Y = begData.Y;
    ncommand->Z = begData.Z;
    ncommand->A = begData.A;

    ncommand->splits = n;
    ncommand->accelCode = ACCELERAT_CODE;

    // now split
    bool endLoop = false;

    for (int step = 0; step < n; ++step) {
        //coordinates of next arc point
        angle += dAlpha;
        loopPos += dPos;

        float c = cos(angle);
        float s = sin(angle);

        switch (endData->plane) {
            case XY: {
                float x_new = i + r * c;
                float y_new = j + r * s;
                ncommand->angle = atan2(y_new - ncommand->Y, x_new - ncommand->X);
                ncommand->X = x_new;
                ncommand->Y = y_new;
                ncommand->Z = loopPos;
#if DEBUG_ARC
                dbg += QString().sprintf("n=%d x=%f y=%f angle=%f sin=%f cos=%f\n", step, x_new, y_new, angle, s, c);
#endif

                if (sqrt((x_new - endData->X) * (x_new - endData->X) + (y_new - endData->Y) * (y_new - endData->Y)) <= splitLen) {
                    ncommand->angle = atan2(y_new - endData->Y, x_new - endData->X);
                    endLoop = true;
                }
            }
            break;

            case YZ: {
                float y_new = j + r * c;
                float z_new = k + r * s;
                ncommand->angle = atan2(z_new - ncommand->Z, y_new - ncommand->Y);
                ncommand->Y = y_new;
                ncommand->Z = z_new;
                ncommand->X = loopPos;
#if DEBUG_ARC
                dbg += QString().sprintf("n=%d y=%f z=%f angle=%f sin=%f cos=%f\n", step, y_new, z_new, angle, s, c);
#endif

                if (sqrt((y_new - endData->Y) * (y_new - endData->Y) + (z_new - endData->Z) * (z_new - endData->Z)) <= splitLen) {
                    ncommand->angle = atan2(z_new - endData->Z, y_new - endData->Y);
                    endLoop = true;
                }
            }
            break;

            case ZX: {
                float z_new = k + r * c;
                float x_new = i + r * s;
                ncommand->angle = atan2(x_new - ncommand->X, z_new - ncommand->Z);
                ncommand->Z = z_new;
                ncommand->X = x_new;
                ncommand->Y = loopPos;
#if DEBUG_ARC
                dbg += QString().sprintf("n=%d z=%f x=%f angle=%f sin=%f cos=%f\n", step, z_new, x_new, angle, s, c);
#endif

                if (sqrt((x_new - endData->X) * (x_new - endData->X) + (z_new - endData->Z) * (z_new - endData->Z)) <= splitLen) {
                    ncommand->angle = atan2(x_new - endData->X, z_new - endData->Z);
                    endLoop = true;
                }
            }
            break;

            default:
                qDebug() << "no plane info!";
                break;
        }

        if (endLoop == true) {
            //             ncommand->accelCode = DECELERAT_CODE; //
            ncommand->X = endData->X;
            ncommand->Y = endData->Y;
            ncommand->Z = endData->Z;

            n = step;

            tmpList << *ncommand;

            break;
        }

        tmpList << *ncommand;
        ncommand = new GCodeData(*ncommand);
        ncommand->accelCode = CONSTSPEED_CODE;
        ncommand->splits = 0;
    }


    if (tmpList.length() > 0) {
        tmpList[tmpList.length() - 1].accelCode = DECELERAT_CODE; //
        tmpList[0].splits = n;
    }


    gCodeList.append(tmpList);

    tmpList.clear();

#if DEBUG_ARC

    if ((fabs (x2 - res.last().X) > (bLength / splitsPerMm)) || (fabs (y2 - res.last().Y) > (bLength / splitsPerMm))) { // wenn zu weit vom ziel...
        qDebug() << "begin: " << x1 << y1 << "end" << x2 << y2 << "center" << i << j;
        qDebug() << "bogen " << bLength << "mm" << "r" << r << "a" << a << "triangle alpha" << alpha;
        qDebug() << "alpha:" << alpha_beg << "->" << alpha_end << "d alpha: " << dAlpha; // rad
        qDebug() << dbg;
    }

#endif
}


// if anything is detected, return true
bool GCodeParser::parseArc(const QString &line, Vec3 &pos, float &R, const float coef)
{
    if (line.isEmpty() == true) {
        return false;
    }

    //  qDebug() << line;
    const QStringList &chunks = line.toUpper().simplified().split(' ');

    Vec3 arc(COORD_TOO_BIG, COORD_TOO_BIG, COORD_TOO_BIG); // too big coordinates

    if (chunks.count() == 0) {
        return false;
    }

    bool res = false;

    R = 0.0;

    for(int i = 1 ; i < chunks.size() ; ++i) {
        const QString &s = chunks[i];
        bool conv;

        switch(s[0].toLatin1()) {
            case 'I': {
                arc.x() = pos.x() + coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'J': {
                arc.y() = pos.y() + coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'K': {
                arc.z() = pos.z() + coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'R': {
                R = coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            default:
                break;
        }
    }

    pos = arc;

    return res;
}


// if anything is detected, return true
bool GCodeParser::parseCoord(const QString &line, Vec3 &pos, float &E, const float coef, float *F)
{
    if (line.isEmpty() == true) {
        return false;
    }

    //  qDebug() << line;
    const QStringList &chunks = line.toUpper().simplified().split(' ');

    if (chunks.count() == 0) {
        return false;
    }

    bool res = false;

    for(int i = 1 ; i < chunks.size() ; ++i) {
        const QString &s = chunks[i];
        bool conv;

        switch(s[0].toLatin1()) {
            case 'X': {
                pos.x() = coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'Y': {
                pos.y() = coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'Z': {
                pos.z() = coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'A': // rotation X
            case 'B': // rotation Y
            case 'C': { // rotation Z are not supported
                break;
            }

            case 'E': {
                E = coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'F': {
                if (F) {
                    *F = s.right(s.size() - 1).toDouble(&conv);
                }

                if (conv == true) {
                    res = true;
                }

                break;
            }

            default:
                break;
        }
    }

    return res;
}


QStringList GCodeParser::getGoodList()
{
    return goodList;
}


QStringList GCodeParser::getBadList()
{
    return badList;
}
