/*****************************************************************************
 *
 * Copyright (c) 2010, the Trustees of Indiana University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Indiana University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE TRUSTEES OF INDIANA UNIVERSITY ''AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE TRUSTEES OF INDIANA UNIVERSITY BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 ***************************************************************************/

#ifndef DYNAMIC_PATH_H
#define DYNAMIC_PATH_H

#include "ParabolicRamp.h"

/** @brief A base class for a feasibility checker.
 */
class FeasibilityCheckerBase
{
 public:
  virtual ~FeasibilityCheckerBase();
  virtual bool ConfigFeasible(const Vector& x)=0;
  virtual bool SegmentFeasible(const Vector& a,const Vector& b)=0;
};

/** @brief A base class for a distance checker.
 * ObstacleDistance returns the radius of a L-z norm guaranteed to
 * be collision-free.  ObstacleDistanceNorm returns the value of z.
 *
 * DynamicPath can currently only handle L-Inf norms.
 */
class DistanceCheckerBase
{
 public:
  virtual ~DistanceCheckerBase();
  virtual Real ObstacleDistanceNorm() const { return Inf; }
  virtual Real ObstacleDistance(const Vector& x)=0;
};

/** @brief A bounded-velocity, bounded-acceleration trajectory consisting
 * of parabolic ramps.
 */
class DynamicPath
{
 public:
  DynamicPath();
  void Init(const Vector& velMax,const Vector& accMax);
  inline void Clear() { ramps.clear(); }
  inline bool Empty() const { return ramps.empty(); }
  Real GetTotalTime() const;
  void Evaluate(Real t,Vector& x);
  void Derivative(Real t,Vector& dx);
  void SetMilestones(const std::vector<Vector>& x);
  void SetMilestones(const std::vector<Vector>& x,const std::vector<Vector>& dx);
  void GetMilestones(std::vector<Vector>& x,std::vector<Vector>& dx) const;
  void Append(const Vector& x);
  void Append(const Vector& x,const Vector& dx);
  bool TryShortcut(Real t1,Real t2,FeasibilityCheckerBase*,Real tol);
  bool TryShortcut(Real t1,Real t2,FeasibilityCheckerBase*,DistanceCheckerBase*);
  int Shortcut(int numIters,FeasibilityCheckerBase*,Real tol);
  int ShortCircuit(FeasibilityCheckerBase*,Real tol);
  int Shortcut(int numIters,FeasibilityCheckerBase*,DistanceCheckerBase*);
  int ShortCircuit(FeasibilityCheckerBase*,DistanceCheckerBase*);

  /// The velocity and acceleration bounds
  Vector velMax,accMax;
  /// The path is stored as a series of ramps
  std::vector<ParabolicRampND> ramps;
};

#endif
