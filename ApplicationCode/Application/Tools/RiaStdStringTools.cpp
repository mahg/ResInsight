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

#include "RiaStdStringTools.h"

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::string RiaStdStringTools::trimString(const std::string& s)
{
    auto sCopy = s.substr(0, s.find_last_not_of(' ') + 1);
    sCopy = sCopy.substr(sCopy.find_first_not_of(' '));

    return sCopy;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RiaStdStringTools::isNumber(const std::string& s)
{
    return (s.find_first_not_of("0123456789.eE-") != std::string::npos);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
int RiaStdStringTools::toInt(const std::string& s)
{
    int intValue = -1;

    try
    {
        intValue = std::stoi(s);
    }
    catch (...)
    {
    }

    return intValue;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double RiaStdStringTools::toDouble(const std::string& s)
{
    double doubleValue = -1.0;

    char* end;
    doubleValue = std::strtod(s.data(), &end);

    return doubleValue;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<std::string> RiaStdStringTools::splitStringBySpace(const std::string& s)
{
    std::vector<std::string> words;

    splitByDelimiter(s, words);

    return words;
}
