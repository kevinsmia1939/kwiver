/*ckwg +29
 * Copyright 2017 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief associate_detections_to_tracks algorithm definition
 */

#ifndef VITAL_ALGO_ASSOCIATE_DETECTIONS_TO_TRACKS_H_
#define VITAL_ALGO_ASSOCIATE_DETECTIONS_TO_TRACKS_H_

#include <vital/vital_config.h>
#include <vital/algo/algorithm.h>

#include <vital/types/object_track_set.h>
#include <vital/types/detection_set.h>
#include <vital/types/image_container.h>
#include <vital/types/matrix.h>

namespace kwiver {
namespace vital {
namespace algo {

/// An abstract base class for computing association cost matrices for tracking
class VITAL_ALGO_EXPORT associate_detections_to_tracks
  : public kwiver::vital::algorithm_def<associate_detections_to_tracks>
{
public:
  /// Return the name of this algorithm
  static std::string static_type_name() { return "associate_detections_to_tracks"; }

  /// Compute an association matrix given detections and tracks
  /**
   * \param fid frame ID
   * \param image contains the input image for the current frame
   * \param tracks active track set from the last frame
   * \param detections detected object sets from the current frame
   * \param detections detected object sets from the current frame
   * \returns a new updated track set
   */
  kwiver::vital::object_track_set_sptr
  associate( frame_id_t fid,
             kwiver::vital::image_container_sptr image,          
             kwiver::vital::object_track_set_sptr tracks,
             kwiver::vital::detection_set_sptr detections,
             virtual kwiver::vital::matrix_2x2d matrix ) const = 0;

protected:
  associate_detections_to_tracks();

};


/// Shared pointer for associate_detections_to_tracks algorithm definition class
typedef std::shared_ptr<associate_detections_to_tracks> associate_detections_to_tracks_sptr;


} } } // end namespace

#endif // VITAL_ALGO_COMPUTE_STEREO_DEPTH_MAP_H_
