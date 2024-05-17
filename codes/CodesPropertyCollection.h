//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef CODES_PROPERTY_COLLECTION_H
#define CODES_PROPERTY_COLLECTION_H

#include "codes/CodesProperty.h"

#include <map>
#include <string>

namespace codes
{

class CodesPropertyCollection
{
public:
  using PropertiesCollectionType = std::map<std::string, CodesProperty>;

  void SetName(const std::string& name);
  std::string GetName();

  PropertiesCollectionType& GetProperties();

  CodesProperty& MakeProperty(const std::string& name, PropertyType type);
  CodesProperty& MakeProperty(const std::string& name, PropertyType type, unsigned int numElements);

  void Clear();

  bool Has(const std::string& name);

  CodesProperty& GetProperty(const std::string& name);

  void Set(const std::string& name, bool value, unsigned int index = 0);
  bool GetBool(const std::string& name, unsigned int index = 0);
  std::vector<bool>& GetBoolElements(const std::string& name);

  void Set(const std::string& name, int value, unsigned int index = 0);
  int GetInt(const std::string& name, unsigned int index = 0);
  std::vector<int>& GetIntElements(const std::string& name);

  void Set(const std::string& name, double value, unsigned int index = 0);
  double GetDouble(const std::string& name, unsigned int index = 0);
  std::vector<double>& GetDoubleElements(const std::string& name);

  void Set(const std::string& name, const std::string& value, unsigned int index = 0);
  std::string GetString(const std::string& name);
  std::vector<std::string>& GetStringElements(const std::string& name);

private:
  PropertiesCollectionType Properties;
  std::string Name;
};

} // end namespace codes

#endif // CODES_PROPERTY_COLLECTION_H
