//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include "codes/CodesProperty.h"

#include <iostream>

namespace codes
{

CodesProperty::CodesProperty()
  : Type(PropertyType::Int)
  , NumberOfElements(0)
{
}

CodesProperty::CodesProperty(PropertyType type)
  : NumberOfElements(0)
{
  this->SetPropertyType(type);
}

CodesProperty::CodesProperty(PropertyType type, unsigned int numElements)
{
  this->SetPropertyType(type);
  this->SetNumberOfElements(numElements);
}

void CodesProperty::SetPropertyType(PropertyType type)
{
  if (this->Type == type)
  {
    return;
  }

  this->Type = type;
  switch (this->Type)
  {
    case PropertyType::Bool:
      this->BoolValues.resize(this->NumberOfElements);
      this->IntValues.clear();
      this->DoubleValues.clear();
      this->StringValues.clear();
      break;
    case PropertyType::Int:
      this->BoolValues.clear();
      this->IntValues.resize(this->NumberOfElements);
      this->DoubleValues.clear();
      this->StringValues.clear();
      break;
    case PropertyType::Double:
      this->BoolValues.clear();
      this->IntValues.clear();
      this->DoubleValues.resize(this->NumberOfElements);
      this->StringValues.clear();
      break;
    case PropertyType::String:
      this->BoolValues.clear();
      this->IntValues.clear();
      this->DoubleValues.clear();
      this->StringValues.resize(this->NumberOfElements);
      break;
    default:
      std::cout << "warning" << std::endl;
  }
}

PropertyType CodesProperty::GetPropertyType()
{
  return this->Type;
}

void CodesProperty::SetNumberOfElements(unsigned int numElements)
{
  if (this->NumberOfElements == numElements)
  {
    return;
  }

  this->NumberOfElements = numElements;
  switch (this->Type)
  {
    case PropertyType::Bool:
      this->BoolValues.clear();
      this->BoolValues.resize(this->NumberOfElements);
      break;
    case PropertyType::Int:
      this->IntValues.clear();
      this->IntValues.resize(this->NumberOfElements);
      break;
    case PropertyType::Double:
      this->DoubleValues.clear();
      this->DoubleValues.resize(this->NumberOfElements);
      break;
    case PropertyType::String:
      this->StringValues.clear();
      this->StringValues.resize(this->NumberOfElements);
      break;
    default:
      std::cout << "warning" << std::endl;
  }
}

unsigned int CodesProperty::GetNumberOfElements()
{
  return this->NumberOfElements;
}

/***** Set/Get for bool *****/

void CodesProperty::Set(bool value, unsigned int index)
{
  assert(this->Type == PropertyType::Bool);
  assert(index < this->BoolValues.size());
  this->BoolValues[index] = value;
}

bool CodesProperty::GetBool(unsigned int index)
{
  assert(this->Type == PropertyType::Bool);
  assert(index < this->BoolValues.size());
  return this->BoolValues[index];
}

std::vector<bool>& CodesProperty::GetBoolElements()
{
  assert(this->Type == PropertyType::Bool);
  return this->BoolValues;
}

/***** Set/Get for int *****/

void CodesProperty::Set(int value, unsigned int index)
{
  assert(this->Type == PropertyType::Int);
  assert(index < this->IntValues.size());
  this->IntValues[index] = value;
}

int CodesProperty::GetInt(unsigned int index)
{
  assert(this->Type == PropertyType::Int);
  assert(index < this->IntValues.size());
  return this->IntValues[index];
}

std::vector<int>& CodesProperty::GetIntElements()
{
  assert(this->Type == PropertyType::Int);
  return this->IntValues;
}

/***** Set/Get for double *****/

void CodesProperty::Set(double value, unsigned int index)
{
  assert(this->Type == PropertyType::Double);
  assert(index < this->DoubleValues.size());
  this->DoubleValues[index] = value;
}

double CodesProperty::GetDouble(unsigned int index)
{
  assert(this->Type == PropertyType::Double);
  assert(index < this->DoubleValues.size());
  return this->DoubleValues[index];
}

std::vector<double>& CodesProperty::GetDoubleElements()
{
  assert(this->Type == PropertyType::Double);
  return this->DoubleValues;
}

/***** Set/Get for std::string *****/

void CodesProperty::Set(const std::string& value, unsigned int index)
{
  assert(this->Type == PropertyType::String);
  assert(index < this->StringValues.size());
  this->StringValues[index] = value;
}

std::string CodesProperty::GetString(unsigned int index)
{
  assert(this->Type == PropertyType::String);
  assert(index < this->StringValues.size());
  return this->StringValues[index];
}

std::vector<std::string>& CodesProperty::GetStringElements()
{
  assert(this->Type == PropertyType::String);
  return this->StringValues;
}

} // end namespace codes
