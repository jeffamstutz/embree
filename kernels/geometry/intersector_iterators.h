// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#pragma once

#include "../common/scene.h"
#include "../common/ray.h"
#include "../common/accel.h"
#include "../geometry/primitive.h"

namespace embree
{
  namespace isa
  {
    template<
      typename Intersector1,
      typename Intersector2,
      typename Intersector3,
      typename Intersector4>

      struct Virtual4LeafIntersector1
      {
        typedef void* Primitive;

        struct Precalculations
        {
          __forceinline Precalculations (const Ray& ray, const AccelData* accel)
            : leaf_intersector(accel->leaf_intersector), pre0(ray,accel), pre1(ray,accel), pre2(ray,accel), pre3(ray,accel) 
          {
            table[0] = &pre0;
            table[1] = &pre1;
            table[2] = &pre2;
            table[3] = &pre3;
          }
        
        public:
          const LeafIntersector* leaf_intersector;
          typename Intersector1::Precalculations pre0;
          typename Intersector1::Precalculations pre1;
          typename Intersector1::Precalculations pre2;
          typename Intersector1::Precalculations pre3;
          void* table[4];
        };

        static __forceinline void intersect(Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node)
        {
          const unsigned int ty = (unsigned int) Leaf::decodeTy(*(unsigned int*)prim);
          assert(ty < 4);
          pre.leaf_intersector->vtable1[ty].intersect(pre.table[ty],ray,context,prim,num,lazy_node);
        }
        
        static __forceinline bool occluded(Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node) 
        {
          const unsigned int ty = (unsigned int) Leaf::decodeTy(*(unsigned int*)prim);
          assert(ty < 4);
          return pre.leaf_intersector->vtable1[ty].occluded(pre.table[ty],ray,context,prim,num,lazy_node);
        }
      };

    template<typename Intersector>
      struct ArrayIntersector1
      {
        typedef typename Intersector::Primitive Primitive;
        typedef typename Intersector::Precalculations Precalculations;

        static const bool validIntersectorK = false; // FIXME: why do we need this

        static __forceinline void intersect(Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node)
        {
          for (size_t i=0; i<num; i++)
            Intersector::intersect(pre,ray,context,prim[i]);
        }

        static __forceinline void vintersect(void* pre, Ray& ray, IntersectContext* context, const void* prim, size_t num, size_t& lazy_node) {
          intersect((Precalculations&)pre,ray,context,(const Primitive*)prim,num,lazy_node);
        }
        
        static __forceinline bool occluded(Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node) 
        {
          for (size_t i=0; i<num; i++) {
            if (Intersector::occluded(pre,ray,context,prim[i]))
              return true;
          }
          return false;
        }

        static __forceinline bool voccluded(void* pre, Ray& ray, IntersectContext* context, const void* prim, size_t num, size_t& lazy_node) {
          return occluded((Precalculations&)pre,ray,context,(const Primitive*)prim,num,lazy_node);
        }

        static __forceinline size_t intersect(Precalculations* pre, size_t valid, Ray** rays, IntersectContext* context,  const Primitive* prim, size_t num, size_t& lazy_node)
        {
          size_t valid_isec = 0;
          do {
            const size_t i = __bscf(valid);
            const float old_far = rays[i]->tfar;
            intersect(pre[i],*rays[i],context,prim,num,lazy_node);
            valid_isec |= (rays[i]->tfar < old_far) ? ((size_t)1 << i) : 0;            
          } while(unlikely(valid));
          return valid_isec;
        }

        static __forceinline size_t occluded(Precalculations* pre, size_t valid, Ray** rays, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node) 
        {
          size_t hit = 0;
          do {
            const size_t i = __bscf(valid);            
            if (occluded(pre[i],*rays[i],context,prim,num,lazy_node))
            {
              hit |= (size_t)1 << i;
              rays[i]->geomID = 0;
            }
          } while(valid);

          return hit;
        }


        template<int K>
        static __forceinline void intersectK(const vbool<K>& valid, /* PrecalculationsK& pre, */ RayK<K>& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node)
        {
        }

        template<int K>        
        static __forceinline vbool<K> occludedK(const vbool<K>& valid, /* PrecalculationsK& pre, */ RayK<K>& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node)
        {
          return valid;
        }
      };

    template<int K, typename Intersector>
      struct ArrayIntersectorK_1 
      {
        typedef typename Intersector::Primitive Primitive;
        typedef typename Intersector::Precalculations Precalculations;
        
        static __forceinline void intersect(const vbool<K>& valid, Precalculations& pre, RayK<K>& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node)
        {
          for (size_t i=0; i<num; i++) {
            Intersector::intersect(valid,pre,ray,context,prim[i]);
          }
        }
        
        static __forceinline vbool<K> occluded(const vbool<K>& valid, Precalculations& pre, RayK<K>& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node) 
        {
          vbool<K> valid0 = valid;
          for (size_t i=0; i<num; i++) {
            valid0 &= !Intersector::occluded(valid0,pre,ray,context,prim[i]);
            if (none(valid0)) break;
          }
          return !valid0;
        }
        
        static __forceinline void intersect(Precalculations& pre, RayK<K>& ray, size_t k, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node)
        {
          for (size_t i=0; i<num; i++) {
            Intersector::intersect(pre,ray,k,context,prim[i]);
          }
        }
        
        static __forceinline bool occluded(Precalculations& pre, RayK<K>& ray, size_t k, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node) 
        {
          for (size_t i=0; i<num; i++) {
            if (Intersector::occluded(pre,ray,k,context,prim[i]))
              return true;
          }
          return false;
        }
      };

    // =============================================================================================

    template<int K, typename Intersector1, typename IntersectorK>
      struct ArrayIntersectorKStream 
      {
        typedef typename Intersector1::Primitive Primitive;
        typedef typename Intersector1::Precalculations Precalculations;
        typedef typename IntersectorK::Primitive PrimitiveK;
        typedef typename IntersectorK::Precalculations PrecalculationsK;

        static const bool validIntersectorK = true;
        
        static __forceinline void intersectK(const vbool<K>& valid, /* PrecalculationsK& pre, */ RayK<K>& ray, IntersectContext* context, const PrimitiveK* prim, size_t num, size_t& lazy_node)
        {
          PrecalculationsK pre(valid,ray); // FIXME: might cause trouble

          for (size_t i=0; i<num; i++) {
            IntersectorK::intersect(valid,pre,ray,context,prim[i]);
          }
        }
        
        static __forceinline vbool<K> occludedK(const vbool<K>& valid, /* PrecalculationsK& pre, */ RayK<K>& ray, IntersectContext* context, const PrimitiveK* prim, size_t num, size_t& lazy_node)
        {
          PrecalculationsK pre(valid,ray); // FIXME: might cause trouble
          vbool<K> valid0 = valid;
          for (size_t i=0; i<num; i++) {
            valid0 &= !IntersectorK::occluded(valid0,pre,ray,context,prim[i]);
            if (none(valid0)) break;
          }
          return !valid0;
        }

        static __forceinline void intersect(Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node)
        {
          for (size_t i=0; i<num; i++)
            Intersector1::intersect(pre,ray,context,prim[i]);
        }
        
        static __forceinline bool occluded(Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node) 
        {
          for (size_t i=0; i<num; i++) {
            if (Intersector1::occluded(pre,ray,context,prim[i]))
              return true;
          }
          return false;
        }

        static __forceinline size_t intersect(Precalculations* pre, size_t valid, Ray** rays, IntersectContext* context,  const Primitive* prim, size_t num, size_t& lazy_node)
        {
          size_t valid_isec = 0;
          do {
            const size_t i = __bscf(valid);
            const float old_far = rays[i]->tfar;
            intersect(pre[i],*rays[i],context,prim,num,lazy_node);
            valid_isec |= (rays[i]->tfar < old_far) ? ((size_t)1 << i) : 0;            
          } while(unlikely(valid));
          return valid_isec;
        }

        static __forceinline size_t occluded(Precalculations* pre, size_t valid, Ray** rays, IntersectContext* context, const Primitive* prim, size_t num, size_t& lazy_node) 
        {
          size_t hit = 0;
          do {
            const size_t i = __bscf(valid);            
            if (occluded(pre[i],*rays[i],context,prim,num,lazy_node))
            {
              hit |= (size_t)1 << i;
              rays[i]->geomID = 0;
            }
          } while(valid);

          return hit;
        }
      };
  }
}
