// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "../math/box_utility.ih"
#include "../value_selector/ValueSelector.ih"
#include "../volume/GridAccelerator.ih"
#include "../volume/SharedStructuredVolume.ih"
#include "GridAcceleratorIterator.ih"

inline void printGridAcceleratorIterator(
    varying GridAcceleratorIterator *uniform self)
{
  print("origin:\n  %\n  %\n  %\n",
        self->origin.x,
        self->origin.y,
        self->origin.z);
  print("direction:\n  %\n  %\n  %\n",
        self->direction.x,
        self->direction.y,
        self->direction.z);
  print("tRange\n  %\n  %\n", self->tRange.lower, self->tRange.upper);
  print("boundingBoxTRange:\n  %\n  %\n",
        self->boundingBoxTRange.lower,
        self->boundingBoxTRange.upper);
}

export uniform int EXPORT_UNIQUE(GridAcceleratorIteratorU_sizeOf)
{
  return sizeof(uniform GridAcceleratorIterator);
}

export uniform int EXPORT_UNIQUE(GridAcceleratorIteratorV_sizeOf)
{
  return sizeof(varying GridAcceleratorIterator);
}

#define template_GridAcceleratorIteratorU_Initialize_internal(univary)         \
  univary GridAcceleratorIterator *uniform self =                              \
      (univary GridAcceleratorIterator * uniform) _self;                       \
                                                                               \
  self->volume        = (uniform SharedStructuredVolume * uniform) _volume;    \
  self->origin        = *((univary vec3f * uniform) _origin);                  \
  self->direction     = *((univary vec3f * uniform) _direction);               \
  self->tRange        = *((univary box1f * uniform) _tRange);                  \
  self->valueSelector = (uniform ValueSelector * uniform) _valueSelector;      \
                                                                               \
  self->boundingBoxTRange = intersectBox(                                      \
      self->origin, self->direction, self->volume->boundingBox, self->tRange); \
                                                                               \
  /* if using ISPC fast-math and approximate rcp() functions, an epsilon needs \
   to be added to the bounding box intersection to prevent artifacts. this is  \
   because the inverse coordinate transformation may land outside a grid cell. \
   the artifacts may look like speckles, or cross-hairs in orthographic camera \
   views. */                                                                   \
  /*const uniform float epsilon = reduce_min(self->volume->gridSpacing) *      \
  0.125f;                                                                      \
                                                                               \
  self->boundingBoxTRange.lower += epsilon;*/                                  \
                                                                               \
  resetInterval(self->intervalState.currentInterval);                          \
  self->intervalState.currentCellIndex = make_vec3i(-1);                       \
                                                                               \
  /* compute interval nominal deltaT based on gridSpacing and direction; the   \
   below is equivalent to: dot(abs(normalize(direction)), gridSpacing) /       \
   length(direction)*/                                                         \
  self->intervalState.currentInterval.nominalDeltaT =                          \
      dot(absf(self->direction), self->volume->gridSpacing) /                  \
      dot(self->direction, self->direction);                                   \
                                                                               \
  self->hitState.currentCellIndex  = make_vec3i(-1);                           \
  self->hitState.currentCellTRange = make_box1f(inf, -inf);

export void EXPORT_UNIQUE(GridAcceleratorIteratorU_Initialize,
                          void *uniform _self,
                          void *uniform _volume,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          void *uniform _valueSelector)
{
  template_GridAcceleratorIteratorU_Initialize_internal(uniform);
}

export void EXPORT_UNIQUE(GridAcceleratorIteratorV_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _volume,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          void *uniform _valueSelector)
{
  if (!imask[programIndex]) {
    return;
  }

  template_GridAcceleratorIteratorU_Initialize_internal(varying);
}
#undef template_GridAcceleratorIteratorU_Initialize_internal

export void *uniform EXPORT_UNIQUE(GridAcceleratorIteratorU_getCurrentInterval,
                                   void *uniform _self)
{
  uniform GridAcceleratorIterator *uniform self =
      (uniform GridAcceleratorIterator * uniform) _self;
  return &self->intervalState.currentInterval;
}

export void *uniform EXPORT_UNIQUE(GridAcceleratorIteratorV_getCurrentInterval,
                                   void *uniform _self)
{
  varying GridAcceleratorIterator *uniform self =
      (varying GridAcceleratorIterator * uniform) _self;
  return &self->intervalState.currentInterval;
}

#define template_GridAcceleratorIteratorU_iterateInterval_internal(univary)   \
  univary GridAcceleratorIterator *uniform self =                             \
      (univary GridAcceleratorIterator * uniform) _self;                      \
                                                                              \
  univary int *uniform result = (univary int *uniform)_result;                \
                                                                              \
  if (isempty1f(self->boundingBoxTRange)) {                                   \
    *result = false;                                                          \
    return;                                                                   \
  }                                                                           \
                                                                              \
  while (                                                                     \
      GridAccelerator_nextCell(self->volume->accelerator,                     \
                               self,                                          \
                               self->intervalState.currentCellIndex,          \
                               self->intervalState.currentInterval.tRange)) { \
    univary box1f cellValueRange;                                             \
    GridAccelerator_getCellValueRange(self->volume->accelerator,              \
                                      self->intervalState.currentCellIndex,   \
                                      cellValueRange);                        \
                                                                              \
    univary bool returnInterval = false;                                      \
                                                                              \
    if (!self->valueSelector) {                                               \
      returnInterval = true;                                                  \
    } else {                                                                  \
      if (overlaps1f(self->valueSelector->rangesMinMax, cellValueRange)) {    \
        if (overlapsAny1f(cellValueRange,                                     \
                          self->valueSelector->numRanges,                     \
                          self->valueSelector->ranges)) {                     \
          returnInterval = true;                                              \
        }                                                                     \
      }                                                                       \
    }                                                                         \
                                                                              \
    if (returnInterval) {                                                     \
      self->intervalState.currentInterval.valueRange = cellValueRange;        \
                                                                              \
      /* nominalDeltaT is set during iterator initialization */               \
                                                                              \
      *result = true;                                                         \
      return;                                                                 \
    }                                                                         \
  }                                                                           \
                                                                              \
  *result = false;

export void EXPORT_UNIQUE(GridAcceleratorIteratorU_iterateInterval,
                          void *uniform _self,
                          uniform int *uniform _result)
{
  template_GridAcceleratorIteratorU_iterateInterval_internal(uniform);
}

export void EXPORT_UNIQUE(GridAcceleratorIteratorV_iterateInterval,
                          const int *uniform imask,
                          void *uniform _self,
                          uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  template_GridAcceleratorIteratorU_iterateInterval_internal(varying);
}
#undef template_GridAcceleratorIteratorU_iterateInterval_internal

export void *uniform EXPORT_UNIQUE(GridAcceleratorIteratorU_getCurrentHit,
                                   void *uniform _self)
{
  uniform GridAcceleratorIterator *uniform self =
      (uniform GridAcceleratorIterator * uniform) _self;
  return &self->hitState.currentHit;
}

export void *uniform EXPORT_UNIQUE(GridAcceleratorIteratorV_getCurrentHit,
                                   void *uniform _self)
{
  varying GridAcceleratorIterator *uniform self =
      (varying GridAcceleratorIterator * uniform) _self;
  return &self->hitState.currentHit;
}

#define template_GridAcceleratorIterator_iterateHit_internal(univary)       \
  univary GridAcceleratorIterator *uniform self =                           \
      (univary GridAcceleratorIterator * uniform) _self;                    \
                                                                            \
  univary int *uniform result = (univary int *uniform)_result;              \
                                                                            \
  if (isempty1f(self->boundingBoxTRange)) {                                 \
    *result = false;                                                        \
    return;                                                                 \
  }                                                                         \
                                                                            \
  cif(!self->valueSelector || self->valueSelector->numValues == 0)          \
  {                                                                         \
    *result = false;                                                        \
    return;                                                                 \
  }                                                                         \
                                                                            \
  /* first iteration */                                                     \
  cif(self->hitState.currentCellIndex.x == -1)                              \
  {                                                                         \
    self->hitState.activeCell =                                             \
        GridAccelerator_nextCell(self->volume->accelerator,                 \
                                 self,                                      \
                                 self->hitState.currentCellIndex,           \
                                 self->hitState.currentCellTRange);         \
  }                                                                         \
                                                                            \
  const uniform float step = reduce_min(self->volume->gridSpacing);         \
                                                                            \
  while (self->hitState.activeCell) {                                       \
    univary box1f cellValueRange;                                           \
    GridAccelerator_getCellValueRange(self->volume->accelerator,            \
                                      self->hitState.currentCellIndex,      \
                                      cellValueRange);                      \
                                                                            \
    univary bool cellValueRangeOverlap =                                    \
        overlaps1f(self->valueSelector->valuesMinMax, cellValueRange);      \
                                                                            \
    if (cellValueRangeOverlap) {                                            \
      univary float surfaceEpsilon;                                         \
                                                                            \
      univary bool foundHit =                                               \
          intersectSurfaces(&self->volume->super,                           \
                            self->origin,                                   \
                            self->direction,                                \
                            self->hitState.currentCellTRange,               \
                            0.5f * step,                                    \
                            self->valueSelector->numValues,                 \
                            self->valueSelector->values,                    \
                            self->hitState.currentHit,                      \
                            surfaceEpsilon);                                \
                                                                            \
      if (foundHit) {                                                       \
        *result = true;                                                     \
        self->hitState.currentCellTRange.lower =                            \
            self->hitState.currentHit.t + surfaceEpsilon;                   \
                                                                            \
        /* move to next cell if next t passes the cell boundary */          \
        if (isempty1f(self->hitState.currentCellTRange)) {                  \
          self->hitState.activeCell =                                       \
              GridAccelerator_nextCell(self->volume->accelerator,           \
                                       self,                                \
                                       self->hitState.currentCellIndex,     \
                                       self->hitState.currentCellTRange);   \
                                                                            \
          /* continue where we left off */                                  \
          self->hitState.currentCellTRange.lower =                          \
              self->hitState.currentHit.t + surfaceEpsilon;                 \
        }                                                                   \
                                                                            \
        return;                                                             \
      }                                                                     \
    }                                                                       \
                                                                            \
    /* if no hits are found, move to the next cell; if a hit is found we'll \
     stay in the cell to pursue other hits */                               \
    self->hitState.activeCell =                                             \
        GridAccelerator_nextCell(self->volume->accelerator,                 \
                                 self,                                      \
                                 self->hitState.currentCellIndex,           \
                                 self->hitState.currentCellTRange);         \
  }                                                                         \
                                                                            \
  *result = false;

export void EXPORT_UNIQUE(GridAcceleratorIteratorU_iterateHit,
                          void *uniform _self,
                          uniform int *uniform _result)
{
  template_GridAcceleratorIterator_iterateHit_internal(uniform);
}

export void EXPORT_UNIQUE(GridAcceleratorIteratorV_iterateHit,
                          const int *uniform imask,
                          void *uniform _self,
                          uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  template_GridAcceleratorIterator_iterateHit_internal(varying);
}
#undef template_GridAcceleratorIterator_iterateHit_internal
