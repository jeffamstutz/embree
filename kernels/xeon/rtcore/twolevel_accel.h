// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#ifndef __EMBREE_TWOLEVEL_ACCEL_H__
#define __EMBREE_TWOLEVEL_ACCEL_H__

#include "embree2/rtcore.h"
#include "rtcore/scene_triangle_mesh.h"
#include "rtcore/scene_user_geometry.h"
#include "common/accel.h"

namespace embree
{
  struct TwoLevelAccel : public Accel
  {
    typedef Accel* (*createTriangleMeshAccelTy)(TriangleMeshScene::TriangleMesh*);

  public:
    TwoLevelAccel (const std::string topAccel, Scene* scene, createTriangleMeshAccelTy createTriangleMeshAccel, bool buildUserGeometryAccel);
    ~TwoLevelAccel ();

  public:
    void build(size_t threadIndex, size_t threadCount);
    void buildTriangleAccels(size_t threadIndex, size_t threadCount);
    void buildUserGeometryAccels(size_t threadIndex, size_t threadCount);

  public:
    Scene* scene;
    createTriangleMeshAccelTy createTriangleMeshAccel;
    bool buildUserGeometryAccel;

  public:
    Accel* accel;
    std::vector<Accel*> triangle_accels;
    std::vector<Accel*> enabled_accels;
  };
}

#endif
