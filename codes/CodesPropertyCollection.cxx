//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/CodesPropertyCollection.h"
#include "codes/CodesProperty.h"

#include <string>
#include <utility>
#include <vector>

namespace codes
{

void CodesPropertyCollection::SetName(const std::string& name)
{
  this->Name = name;
}

std::string CodesPropertyCollection::GetName()
{
  return this->Name;
}

CodesPropertyCollection::PropertiesCollectionType& CodesPropertyCollection::GetProperties()
{
  return this->Properties;
}

CodesProperty& CodesPropertyCollection::MakeProperty(
  const std::string& name, PropertyType type, unsigned int numElements)
{
  this->Properties.insert(std::make_pair(name, CodesProperty(type, numElements)));
  return this->Properties[name];
}

CodesProperty& CodesPropertyCollection::MakeProperty(const std::string& name, PropertyType type)
{
  this->Properties.insert(std::make_pair(name, CodesProperty(type, 1)));
  return this->Properties[name];
}

void CodesPropertyCollection::Clear()
{
  this->Properties.clear();
}

bool CodesPropertyCollection::Has(const std::string& name)
{
  return this->Properties.count(name) > 0;
}

CodesProperty& CodesPropertyCollection::GetProperty(const std::string& name)
{
  if (!this->Has(name))
  {
    // TODO: error
  }
  return this->Properties[name];
}

/***** Set/Get for bool *****/

void CodesPropertyCollection::Set(const std::string& name, bool value, unsigned int index)
{
  this->Properties[name].Set(value, index);
}

bool CodesPropertyCollection::GetBool(const std::string& name, unsigned int index)
{
  return this->Properties[name].GetBool(index);
}

std::vector<bool>& CodesPropertyCollection::GetBoolElements(const std::string& name)
{
  return this->Properties[name].GetBoolElements();
}

/***** Set/Get for int *****/

void CodesPropertyCollection::Set(const std::string& name, int value, unsigned int index)
{
  this->Properties[name].Set(value, index);
}

int CodesPropertyCollection::GetInt(const std::string& name, unsigned int index)
{
  return this->Properties[name].GetInt(index);
}

std::vector<int>& CodesPropertyCollection::GetIntElements(const std::string& name)
{
  return this->Properties[name].GetIntElements();
}

/***** Set/Get for double *****/

void CodesPropertyCollection::Set(const std::string& name, double value, unsigned int index)
{
  this->Properties[name].Set(value, index);
}

double CodesPropertyCollection::GetDouble(const std::string& name, unsigned int index)
{
  return this->Properties[name].GetDouble(index);
}

std::vector<double>& CodesPropertyCollection::GetDoubleElements(const std::string& name)
{
  return this->Properties[name].GetDoubleElements();
}

/***** Set/Get for string *****/

void CodesPropertyCollection::Set(
  const std::string& name, const std::string& value, unsigned int index)
{
  this->Properties[name].Set(value, index);
}

std::string CodesPropertyCollection::GetString(const std::string& name)
{
  return this->Properties[name].GetString();
}

std::vector<std::string>& CodesPropertyCollection::GetStringElements(const std::string& name)
{
  return this->Properties[name].GetStringElements();
}

} // end namespace codes
