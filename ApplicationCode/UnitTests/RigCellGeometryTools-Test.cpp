/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"

#include "RigCellGeometryTools.h"
#include "RigMainGrid.h"


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
TEST(RigCellGeometryTools, createPolygonTest)
{
    cvf::Vec3d a  = cvf::Vec3d(1, 1, 1);
    cvf::Vec3d b  = cvf::Vec3d(1, 3.14159265359, 1);
    cvf::Vec3d b2 = cvf::Vec3d(1, 3.1415926536, 1);
    cvf::Vec3d c  = cvf::Vec3d(5, 5, 1);
    cvf::Vec3d d  = cvf::Vec3d(-2, 8, 1);

    std::list<std::pair<cvf::Vec3d, cvf::Vec3d>> intersectionLineSegments;
    intersectionLineSegments.push_back({a, b});
    intersectionLineSegments.push_back({b2,c});
    intersectionLineSegments.push_back({c, d});
    intersectionLineSegments.push_back({a, d});

    std::vector<std::vector<cvf::Vec3d>> polygons;

    RigCellGeometryTools::createPolygonFromLineSegments(intersectionLineSegments, polygons);

    EXPECT_EQ(polygons.size(), 1);
    EXPECT_EQ(polygons[0].size(), 5);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
TEST(RigCellGeometryTools, createMultiplePolygonTest)
{
    cvf::Vec3d a1 = cvf::Vec3d(5, 4, 1);
    cvf::Vec3d b1 = cvf::Vec3d(6, 3, 1);
    cvf::Vec3d c1 = cvf::Vec3d(6, 4, 1);

    cvf::Vec3d a2 = cvf::Vec3d(2, 1, 1);
    cvf::Vec3d b2 = cvf::Vec3d(1, 3, 1);
    cvf::Vec3d c2 = cvf::Vec3d(1, 5, 1);



    std::list<std::pair<cvf::Vec3d, cvf::Vec3d>> intersectionLineSegments;
    intersectionLineSegments.push_back({ a1, b1 });
    intersectionLineSegments.push_back({ b1, c1 });

    intersectionLineSegments.push_back({ a2, b2 });
    intersectionLineSegments.push_back({ b2, c2 });

    std::vector<std::vector<cvf::Vec3d>> polygons;

    RigCellGeometryTools::createPolygonFromLineSegments(intersectionLineSegments, polygons);

    EXPECT_EQ(polygons.size(), 2);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
TEST(RigCellGeometryTools, planeHexCellIntersectionTest)
{
    cvf::Vec3d hexCorners[8];
    hexCorners[0] = cvf::Vec3d(0, 0, 0);
    hexCorners[1] = cvf::Vec3d(1, 0, 0);
    hexCorners[2] = cvf::Vec3d(0, 1, 0);
    hexCorners[3] = cvf::Vec3d(0, 0, 1);
    hexCorners[4] = cvf::Vec3d(0, 1, 1);
    hexCorners[5] = cvf::Vec3d(1, 1, 0);
    hexCorners[6] = cvf::Vec3d(1, 0, 1);
    hexCorners[7] = cvf::Vec3d(1, 1, 1);

    std::list<std::pair<cvf::Vec3d, cvf::Vec3d > > intersectionLineSegments;
    bool isCellIntersected = false;
    cvf::Plane fracturePlane;
    
    fracturePlane.setFromPointAndNormal(cvf::Vec3d(0.5, 0.5, 0.5), cvf::Vec3d(1, 0, 0) );
    isCellIntersected = RigCellGeometryTools::planeHexCellIntersection(hexCorners, fracturePlane, intersectionLineSegments);
    EXPECT_TRUE(isCellIntersected);

    fracturePlane.setFromPointAndNormal(cvf::Vec3d(1.5, 1.5, 1.5), cvf::Vec3d(1, 0, 0));
    isCellIntersected = RigCellGeometryTools::planeHexCellIntersection(hexCorners, fracturePlane, intersectionLineSegments);
    EXPECT_FALSE(isCellIntersected);

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
TEST(RigCellGeometryTools, findCellAverageZTest)
{
    cvf::Vec3d hexCorners[8];
 
    hexCorners[0] = cvf::Vec3d(0, 0, 0);
    hexCorners[1] = cvf::Vec3d(1, 0, 0);
    hexCorners[2] = cvf::Vec3d(1, 1, 0);
    hexCorners[3] = cvf::Vec3d(0, 1, 0);

    hexCorners[4] = cvf::Vec3d(0, 0, 1);
    hexCorners[5] = cvf::Vec3d(1, 0, 1);
    hexCorners[6] = cvf::Vec3d(1, 1, 1);
    hexCorners[7] = cvf::Vec3d(0, 1, 1);


    cvf::Vec3d localX;
    cvf::Vec3d localY;
    cvf::Vec3d localZ;

    RigCellGeometryTools::findCellLocalXYZ(hexCorners, localX, localY, localZ);

    EXPECT_DOUBLE_EQ(localX[0], 1);
    EXPECT_DOUBLE_EQ(localX[1], 0);
    EXPECT_DOUBLE_EQ(localX[2], 0);

    EXPECT_DOUBLE_EQ(localY[0], 0);
    EXPECT_DOUBLE_EQ(localY[1], 1);
    EXPECT_DOUBLE_EQ(localY[2], 0);

    EXPECT_DOUBLE_EQ(localZ[0], 0);
    EXPECT_DOUBLE_EQ(localZ[1], 0);
    EXPECT_DOUBLE_EQ(localZ[2], 1);

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
TEST(RigCellGeometryTools, lengthCalcTest)
{

    std::vector<cvf::Vec3d> polygonExample;

    polygonExample.push_back(cvf::Vec3d(0.00, 0.00, 0.0));
    polygonExample.push_back(cvf::Vec3d(0.00, 2.50, 0.0));
    polygonExample.push_back(cvf::Vec3d(1.50, 2.50, 0.0));
    polygonExample.push_back(cvf::Vec3d(1.50, 0.00, 0.0));


//     polygonExample.push_back(cvf::Vec3d(0.00, 0.50, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-7.73, 0.48, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-14.69, 0.40, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-20.23, 0.29, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-23.78, 0.15, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-25.00, 0.00, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-23.78, -0.15, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-20.23, -0.29, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-14.69, -0.40, 0.0));
//     polygonExample.push_back(cvf::Vec3d(-7.73, -0.48, 0.0));
//     polygonExample.push_back(cvf::Vec3d(0.00, -0.50, 0.0));
//     polygonExample.push_back(cvf::Vec3d(7.73, -0.48, 0.0));
//     polygonExample.push_back(cvf::Vec3d(14.69, -0.40, 0.0));
//     polygonExample.push_back(cvf::Vec3d(20.23, -0.29, 0.0));
//     polygonExample.push_back(cvf::Vec3d(23.78, -0.15, 0.0));
//     polygonExample.push_back(cvf::Vec3d(25.00, 0.00, 0.0));
//     polygonExample.push_back(cvf::Vec3d(23.78, 0.15, 0.0));
//     polygonExample.push_back(cvf::Vec3d(20.23, 0.29, 0.0));
//     polygonExample.push_back(cvf::Vec3d(14.69, 0.40, 0.0));
//     polygonExample.push_back(cvf::Vec3d(7.73, 0.48, 0.0));
//     polygonExample.push_back(cvf::Vec3d(0.00, 0.50, 0.0));

    double length = 0.0;
    cvf::Vec3d directionOfLength = cvf::Vec3d::ZERO;


    directionOfLength = cvf::Vec3d(1, 0, 0);
    length = RigCellGeometryTools::polygonAreaWeightedLength(directionOfLength, polygonExample);
    EXPECT_DOUBLE_EQ(length, 1.5);

    directionOfLength = cvf::Vec3d(0, 1, 0);
    length = RigCellGeometryTools::polygonAreaWeightedLength(directionOfLength, polygonExample);
    EXPECT_DOUBLE_EQ(length, 2.5);
}