// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <memory>
#include <string>
#include "../common/VKLCommon.h"
#include "../common/simd.h"
#include "openvkl/openvkl.h"
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"
#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/utility/ParameterizedObject.h"

using namespace ospcommon;
using namespace ospcommon::math;

using VKLIntervalIterator1 = VKLIntervalIterator;
using VKLHitIterator1      = VKLHitIterator;

namespace openvkl {
  namespace api {

    struct OPENVKL_CORE_INTERFACE Driver
        : public ospcommon::utility::ParameterizedObject
    {
      static std::shared_ptr<Driver> current;

      Driver();
      virtual ~Driver() override = default;

      static Driver *createDriver(const std::string &driverName);

      // error tracking
      VKLError lastErrorCode       = VKL_NO_ERROR;
      std::string lastErrorMessage = "no error";

      virtual bool supportsWidth(int width) = 0;

      virtual int getNativeSIMDWidth() = 0;

      virtual void commit();
      bool isCommitted();

      virtual void commit(VKLObject object)  = 0;
      virtual void release(VKLObject object) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Driver parameters (updated on commit()) //////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      static VKLLogLevel logLevel;

      std::function<void(const char *)> logFunction{[](const char *) {}};
      std::function<void(VKLError, const char *)> errorFunction{
          [](VKLError, const char *) {}};

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLData newData(size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Observer /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLObserver newObserver(VKLVolume volume, const char *type) = 0;
      virtual const void * mapObserver(VKLObserver observer) = 0;
      virtual void unmapObserver(VKLObserver observer) = 0;
      virtual VKLDataType getObserverElementType(VKLObserver observer) const = 0;
      virtual size_t getObserverNumElements(VKLObserver observer) const = 0;

      /////////////////////////////////////////////////////////////////////////
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual void initIntervalIterator1(vVKLIntervalIteratorN<1> &iterator,
                                         VKLVolume volume,
                                         const vvec3fn<1> &origin,
                                         const vvec3fn<1> &direction,
                                         const vrange1fn<1> &tRange,
                                         VKLValueSelector valueSelector)
      {
        throw std::runtime_error(
            "initIntervalIterator1() not implemented on this driver");
      }

#define __define_initIntervalIteratorN(WIDTH)                            \
  virtual void initIntervalIterator##WIDTH(                              \
      const int *valid,                                                  \
      vVKLIntervalIteratorN<WIDTH> &iterator,                            \
      VKLVolume volume,                                                  \
      const vvec3fn<WIDTH> &origin,                                      \
      const vvec3fn<WIDTH> &direction,                                   \
      const vrange1fn<WIDTH> &tRange,                                    \
      VKLValueSelector valueSelector)                                    \
  {                                                                      \
    throw std::runtime_error(                                            \
        "initIntervalIterator##WIDTH() not implemented on this driver"); \
  }

      __define_initIntervalIteratorN(4);
      __define_initIntervalIteratorN(8);
      __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

      virtual void iterateInterval1(vVKLIntervalIteratorN<1> &iterator,
                                    vVKLIntervalN<1> &interval,
                                    int *result)
      {
        throw std::runtime_error(
            "iterateInterval1() not implemented on this driver");
      }

#define __define_iterateIntervalN(WIDTH)                                      \
  virtual void iterateInterval##WIDTH(const int *valid,                       \
                                      vVKLIntervalIteratorN<WIDTH> &iterator, \
                                      vVKLIntervalN<WIDTH> &interval,         \
                                      int *result)                            \
  {                                                                           \
    throw std::runtime_error(                                                 \
        "iterateInterval##WIDTH() not implemented on this driver");           \
  }

      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual void initHitIterator1(vVKLHitIteratorN<1> &iterator,
                                    VKLVolume volume,
                                    const vvec3fn<1> &origin,
                                    const vvec3fn<1> &direction,
                                    const vrange1fn<1> &tRange,
                                    VKLValueSelector valueSelector)
      {
        throw std::runtime_error(
            "initHitIterator1() not implemented on this driver");
      }

#define __define_initHitIteratorN(WIDTH)                                 \
  virtual void initHitIterator##WIDTH(const int *valid,                  \
                                      vVKLHitIteratorN<WIDTH> &iterator, \
                                      VKLVolume volume,                  \
                                      const vvec3fn<WIDTH> &origin,      \
                                      const vvec3fn<WIDTH> &direction,   \
                                      const vrange1fn<WIDTH> &tRange,    \
                                      VKLValueSelector valueSelector)    \
  {                                                                      \
    throw std::runtime_error(                                            \
        "initHitIterator##WIDTH() not implemented on this driver");      \
  }

      __define_initHitIteratorN(4);
      __define_initHitIteratorN(8);
      __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

      virtual void iterateHit1(vVKLHitIteratorN<1> &iterator,
                               vVKLHitN<1> &hit,
                               int *result)
      {
        throw std::runtime_error(
            "iterateHit1() not implemented on this driver");
      }

#define __define_iterateHitN(WIDTH)                                 \
  virtual void iterateHit##WIDTH(const int *valid,                  \
                                 vVKLHitIteratorN<WIDTH> &iterator, \
                                 vVKLHitN<WIDTH> &hit,              \
                                 int *result)                       \
  {                                                                 \
    throw std::runtime_error(                                       \
        "iterateHit##WIDTH() not implemented on this driver");      \
  }

      __define_iterateHitN(4);
      __define_iterateHitN(8);
      __define_iterateHitN(16);

#undef __define_iterateHitN

      /////////////////////////////////////////////////////////////////////////
      // Module ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLError loadModule(const char *moduleName) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual void setBool(VKLObject object,
                           const char *name,
                           const bool b)                                    = 0;
      virtual void set1f(VKLObject object, const char *name, const float x) = 0;
      virtual void set1i(VKLObject object, const char *name, const int x)   = 0;
      virtual void setVec3f(VKLObject object,
                            const char *name,
                            const vec3f &v)                                 = 0;
      virtual void setVec3i(VKLObject object,
                            const char *name,
                            const vec3i &v)                                 = 0;
      virtual void setObject(VKLObject object,
                             const char *name,
                             VKLObject setObject)                           = 0;
      virtual void setString(VKLObject object,
                             const char *name,
                             const std::string &s)                          = 0;
      virtual void setVoidPtr(VKLObject object, const char *name, void *v)  = 0;

      /////////////////////////////////////////////////////////////////////////
      // Value selector ///////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLValueSelector newValueSelector(VKLVolume volume) = 0;

      virtual void valueSelectorSetRanges(
          VKLValueSelector valueSelector,
          const utility::ArrayView<const range1f> &ranges) = 0;

      virtual void valueSelectorSetValues(
          VKLValueSelector valueSelector,
          const utility::ArrayView<const float> &values) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLVolume newVolume(const char *type) = 0;

#define __define_computeSampleN(WIDTH)                                       \
  virtual void computeSample##WIDTH(const int *valid,                        \
                                    VKLVolume volume,                        \
                                    const vvec3fn<WIDTH> &objectCoordinates, \
                                    float *samples) = 0;

      __define_computeSampleN(1);
      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

#define __define_computeGradientN(WIDTH)                                       \
  virtual void computeGradient##WIDTH(const int *valid,                        \
                                      VKLVolume volume,                        \
                                      const vvec3fn<WIDTH> &objectCoordinates, \
                                      vvec3fn<WIDTH> &gradients) = 0;

      __define_computeGradientN(1);
      __define_computeGradientN(4);
      __define_computeGradientN(8);
      __define_computeGradientN(16);

#undef __define_computeGradientN

      virtual box3f getBoundingBox(VKLVolume volume) = 0;

      virtual range1f getValueRange(VKLVolume volume) = 0;

     private:
      bool committed = false;
    };

    // shorthand functions to query current API device
    OPENVKL_CORE_INTERFACE bool driverIsSet();
    OPENVKL_CORE_INTERFACE Driver &currentDriver();

#define VKL_REGISTER_DRIVER(InternalClass, external_name) \
  VKL_REGISTER_OBJECT(                                    \
      ::openvkl::api::Driver, driver, InternalClass, external_name)

  }  // namespace api
}  // namespace openvkl
