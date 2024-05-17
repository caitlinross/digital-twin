//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_PROPERTY_H
#define CODES_PROPERTY_H

#include <string>
#include <vector>

namespace codes
{

enum class PropertyType
{
  Bool,
  Int,
  Double,
  String
};

class CodesProperty
{
public:
  CodesProperty();
  CodesProperty(PropertyType type);
  CodesProperty(PropertyType type, unsigned int numElements);

  void SetPropertyType(PropertyType type);
  PropertyType GetPropertyType();

  void SetNumberOfElements(unsigned int numElements);
  unsigned int GetNumberOfElements();

  void Set(bool value, unsigned int index = 0);
  bool GetBool(unsigned int index = 0);
  std::vector<bool>& GetBoolElements();

  void Set(int value, unsigned int index = 0);
  int GetInt(unsigned int index = 0);
  std::vector<int>& GetIntElements();

  void Set(double value, unsigned int index = 0);
  double GetDouble(unsigned int index = 0);
  std::vector<double>& GetDoubleElements();

  void Set(const std::string& value, unsigned int index = 0);
  std::string GetString(unsigned int index = 0);
  std::vector<std::string>& GetStringElements();

private:
  PropertyType Type;
  unsigned int NumberOfElements;

  std::vector<bool> BoolValues;
  std::vector<int> IntValues;
  std::vector<double> DoubleValues;
  std::vector<std::string> StringValues;
};

} // end namespace codes

#endif // CODES_PROPERTY_H
