//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================


#ifndef CODES_ORCHESTRATOR_COMPONENT_CONFIG_H
#define CODES_ORCHESTRATOR_COMPONENT_CONFIG_H

namespace codes
{
namespace orchestrator
{
// TODO maybe this isn't a good idea,
// we should probably just go ahead and create the
// LPs
// maybe make some dummy LPs for now for each component type, so we
// can just have something working
// what about the parallel case?
// for now it's not a huge concern since we're doing sims with such small
// number of LPs, but for the future with larger networks
// we'll need to figure out how we want to map LPs to PEs
// need to make sure we do it in a way that makes sense for the graph, but
// I think there's def graph algos out there that can do this partitioning
// I guess we could use metis?

class enum ComponentType
{
  Router,
  Switch,
  Host,
  Unknown
};

class SimulationConfig
{

};

class ComponentConfig
{
public:

protected:
  ComponentType CompType;

private:

};

class HostConfig : public ComponentConfig
{
public:

private:

};

class RouterConfig : public ComponentConfig
{
public:

private:

};

} // end namespace orchestrator
} // end namespace codes

#endif // CODES_ORCHESTRATOR_COMPONENT_CONFIG_H
