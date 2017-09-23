/*ckwg +29
 * Copyright 2016 by Kitware, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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
 * \brief Implementation of blend_images_process
 */

#include "blend_images_process.h"

#include <vital/vital_types.h>
#include <vital/vital_foreach.h>

#include <arrows/ocv/image_container.h>

#include <sprokit/processes/kwiver_type_traits.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <sstream>
#include <iostream>

namespace kwiver {

create_port_trait(image1, image, "First input image" );
create_port_trait(image2, image, "Second input image" );
create_port_trait(blended_image, image, "Blended image" );

create_config_trait( blending_mode, std::string, "linear", 
                     "Blending mode: linear or max." );
create_config_trait( image1_alpha, double, "0.5", "When blending mode is linear,"
                     " this sets the fractional contribution of the blended "
                     "image from image one (range 0-1)." );
create_config_trait( image1_mult, double, "1", "Multiply image one by this "
                     "factor before blending." );
create_config_trait( image1_color_mode, std::string, "RGB", "Describes the "
                     "mapping from source image channel(s) to the RGB blended "
                     "image channels. If the input image has three channels, "
                     "parameter values RGB or BGR describe the channel order. "
                     "If R, G, B, or GRAY is set, the input is converted to "
                     "grayscale (if not already a single channel image) and"
                     "contributes that color to the blended image." );
create_config_trait( image2_alpha, double, "0.5", "When blending mode is linear,"
                     " this sets the fractional contribution of the blended "
                     "image from image two (range 0-1)." );
create_config_trait( image2_mult, double, "1", "Multiply image two by this "
                     "factor before blending." );
create_config_trait( image2_color_mode, std::string, "RGB", "Describes the "
                     "mapping from source image channel(s) to the RGB blended "
                     "image channels. If the input image has three channels, "
                     "parameter values RGB or BGR describe the channel order. "
                     "If R, G, B, or GRAY is set, the input is converted to "
                     "grayscale (if not already a single channel image) and"
                     "contributes that color to the blended image." );


static std::string type2str(int type) 
{
  std::string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}


// ==================================================================
class blend_images_process::priv
{
public:
  std::string m_blending_mode;
  double m_image1_alpha, m_image1_mult;
  std::string m_image1_color_mode;
  double m_image2_alpha, m_image2_mult;
  std::string m_image2_color_mode;
  kwiver::vital::logger_handle_t m_logger;
  
  priv()
    : m_blending_mode("linear"),
      m_image1_alpha(0.5), 
      m_image1_mult(1),
      m_image1_color_mode("RGB"), 
      m_image2_alpha(0.5), 
      m_image2_mult(1),
      m_image2_color_mode("RGB")
  {
  }

  ~priv()
  {
  }
  
  /// Convert vital image container to OpenCV image
  /**
   *
   * \param[in] img   the source vital image container
   * \param[in] cm    the channel or order of channels to populate to
   * \param[out]      OpenCV image
   */
  void
  vital_to_ocv_rgb(kwiver::vital::image_container_sptr img, std::string cm, cv::Mat &img_ocv)
  {
    if( cm == "BGR" )
    {
      img_ocv = arrows::ocv::image_container::vital_to_ocv( img->get_image(), 
                                                            arrows::ocv::image_container::BGR );
    }
    else if( cm == "RGB" )
    {
      img_ocv = arrows::ocv::image_container::vital_to_ocv( img->get_image(), 
                                                            arrows::ocv::image_container::RGB );
    }
    else if( cm == "R" || cm == "G" || cm == "B" || cm == "GRAY" )
    {
      img_ocv = arrows::ocv::image_container::vital_to_ocv( img->get_image() );
      img_ocv = img_ocv.clone();
      if( img_ocv.channels() == 3 )
      {
        cv::cvtColor( img_ocv, img_ocv, CV_RGB2GRAY );
      }
    }
    else
    {
      throw vital::invalid_value( "Invalid image one color mode: " + cm + "!" );
    }
    
    return;
  }
  
  std::array<bool, 3>
  get_channel_map( std::string channel_str )
  {
    if( channel_str == "R" )
    {
      std::array<bool, 3> channel_map{true,false,false};
      return channel_map;
    }
    else if( channel_str == "G" )
    {
      std::array<bool,3 > channel_map{false,true,false};
      return channel_map;
    }
    else if( channel_str == "B" )
    {
      std::array<bool, 3> channel_map{false,false,true};
      return channel_map;
    }
    else
    {
      std::array<bool, 3> channel_map{true,true,true};
      return channel_map;
    }
  }
      
  
    /// Blend a monochrome image with another monochrome image
  /**
   *
   * \param[in] mono1        mono image one
   * \param[in] alpha1       blending factor for mono image two
   * \param[in] chann_str1  "R", "G", "B", or "GRAY" indicating channel blending
   * \param[in] mono1        mono image two
   * \param[in] alpha2       blending factor for mono image two
   * \param[in] chann_str1  "R", "G", "B", or "GRAY" indicating channel blending
   * \return                 blended image
   */
  void
  blend_mono_with_mono( const cv::Mat mono1, const double alpha1, 
                        const std::string chann_str1, 
                        const cv::Mat mono2, const double alpha2, 
                        const std::string chann_str2, cv::Mat &dst )
  {    
    cv::Mat RGB_3[3];
    
    std::array<bool, 3> chann_map1, chann_map2;
    chann_map1 = get_channel_map( chann_str1 );
    chann_map2 = get_channel_map( chann_str2 );
    
    for( std::size_t i=0; i<3; ++i )
    {
      cv::Mat img1, img2;
      if( chann_map1[i] && chann_map2[i] )
      {
        // Both mono images contribute to this ith channel
        if( m_blending_mode == "linear" )
        {
          cv::addWeighted( mono1, alpha1, mono2, alpha2, 0.0, 
                           RGB_3[i], -1 );
        }
        else
        {
          cv::max( mono1, mono2, RGB_3[i] );
        }
      }
      else if( chann_map1[i] )
      {
        // Only mono1 contributes
        if( m_blending_mode == "linear" )
        {
          RGB_3[i] = mono1*alpha1;
        }
        else
        {
          RGB_3[i] = mono1;
        }
      }
      else
      {
        // Only mono2 contributes
        if( m_blending_mode == "linear" )
        {
          RGB_3[i] = mono2*alpha2;
        }
        else
        {
          RGB_3[i] = mono2;
        }
      }
    }
    
    cv::merge(RGB_3, 3, dst);
  }
  
  /// Blend a monochrome image with one channel of an RGB image
  /**
   *
   * \param[in] mono         mono image
   * \param[in] rgb          RGB image
   * \param[in] channel_str  "R", "G", "B", or "GRAY" indicating channel blending
   * \return                 blended image
   */
  void
  blend_mono_with_rgb(const cv::Mat mono, double alpha, const cv::Mat rgb, 
                      const double beta, const std::string channel_str, 
                      cv::Mat &dst)
  {
    std::vector<int> channels;
    if( channel_str == "R" )
    {
      channels.assign({0});
    }
    else if( channel_str == "G" )
    {
      channels.assign({1});
    }
    else if( channel_str == "B" )
    {
      channels.assign({2});
    }
    else
    {
      channels.assign({0,1,2});
    }
    
    cv::Mat RGB_3[3];
    cv::split(rgb, RGB_3);
    
    for( std::size_t i=0; i<channels.size(); ++i )
    {
      if( m_blending_mode == "linear" )
      {
        cv::addWeighted( mono, alpha, RGB_3[channels[i]], beta, 0.0, 
                         RGB_3[channels[i]], -1 );
      }
      else
      {
        cv::max( mono, RGB_3[channels[i]], RGB_3[channels[i]] );
      }
    }
    
    cv::merge(RGB_3, 3, dst);
  }
  
  /// Blend images
  /**
   *
   * \param[in] img1   the first image
   * \param[in] img2   the second image
   * \return           the blended image
   */
  void
  blend_images(cv::Mat img1, cv::Mat img2, cv::Mat &blended_image)
  {
    LOG_TRACE( m_logger, "Blending image one ([" + std::to_string(img1.cols) + 
               ", " + std::to_string(img1.rows) + ", " +
               std::to_string(img1.channels()) + "], " + 
               type2str(img1.type()) + ") with image two ([" + 
               std::to_string(img2.cols) + ", " + std::to_string(img2.rows) + 
               ", " + std::to_string(img2.channels()) + "], " + 
               type2str(img2.type()) + ")");
    
    if( m_image1_mult != 1 )
    {
      img1 = img1*m_image1_mult;
    }
    
    if( m_image2_mult != 1 )
    {
      img2 = img2*m_image2_mult;
    }
    
    if( img1.channels() == 3 && img2.channels() == 3 )
    {
      LOG_TRACE( m_logger, "Both images have 3 channels");
      // If both images have three channels, blend each channel individually.
      if( m_blending_mode == "linear" )
      {
        cv::addWeighted(img1, m_image1_alpha, img2, m_image2_alpha, 0.0, 
                        blended_image, -1);
      }
      else
      {
        cv::max(img1, img2, blended_image);
      }
      return;
    }
    
    if( img1.channels() == 3)  // so img2.channels() != 3
    {
      blend_mono_with_rgb( img2, m_image2_alpha, img1, m_image1_alpha, 
                           m_image2_color_mode, blended_image );
    }
    else if( img2.channels() == 3)  // so img2.channels() != 3
    {
      blend_mono_with_rgb( img1, m_image1_alpha, img2, m_image2_alpha,
                           m_image1_color_mode, blended_image );
    }
    else
    {
      blend_mono_with_mono( img1, m_image1_alpha, m_image1_color_mode, 
                            img2, m_image2_alpha, m_image2_color_mode, 
                            blended_image );
    }
    return;
  }

}; // end priv class


// ==================================================================
blend_images_process
::blend_images_process( vital::config_block_sptr const& config )
  : process( config ),
  d( new blend_images_process::priv )
{
  attach_logger( kwiver::vital::get_logger( name() ) ); // could use a better approach
  d->m_logger = logger();
  make_ports();
  make_config();
}


blend_images_process
  ::~blend_images_process()
{
}


// ------------------------------------------------------------------
void
blend_images_process::_configure()
{
  d->m_blending_mode       = config_value_using_trait( blending_mode );
  d->m_image1_alpha        = config_value_using_trait( image1_alpha );
  d->m_image1_mult         = config_value_using_trait( image1_mult );
  d->m_image1_color_mode   = config_value_using_trait( image1_color_mode );
  d->m_image2_alpha        = config_value_using_trait( image2_alpha );
  d->m_image2_mult         = config_value_using_trait( image2_mult );
  d->m_image2_color_mode   = config_value_using_trait( image2_color_mode );
  
  LOG_DEBUG( d->m_logger, "blending_mode: " + d->m_blending_mode);
  LOG_DEBUG( d->m_logger, "image1_alpha: " + std::to_string(d->m_image1_alpha));
  LOG_DEBUG( d->m_logger, "image1_mult: " + std::to_string(d->m_image1_mult));
  LOG_DEBUG( d->m_logger, "image1_color_mode: " + d->m_image1_color_mode);
  LOG_DEBUG( d->m_logger, "image2_alpha: " + std::to_string(d->m_image2_alpha));
  LOG_DEBUG( d->m_logger, "image2_mult: " + std::to_string(d->m_image2_mult));
  LOG_DEBUG( d->m_logger, "image2_color_mode: " + d->m_image2_color_mode);
} // blend_images_process::_configure


// ------------------------------------------------------------------
void
blend_images_process::_step()
{
  // image
  kwiver::vital::image_container_sptr img1 = grab_from_port_using_trait( image1 );
  kwiver::vital::image_container_sptr img2 = grab_from_port_using_trait( image2 );
  
  LOG_DEBUG( d->m_logger, "Received image one ([" + std::to_string(img1->width()) + 
             ", " + std::to_string(img1->height()) + ", " +
             std::to_string(img1->depth()) + "] with image two ([" + 
             std::to_string(img2->width()) + ", " + std::to_string(img2->height()) + 
             ", " + std::to_string(img2->depth()) + "]");
  
  // --------------------- Convert Input Images to OCV Format ----------------- 
  cv::Mat img1_ocv;
  d->vital_to_ocv_rgb( img1, d->m_image1_color_mode, img1_ocv );
  cv::Mat img2_ocv;
  d->vital_to_ocv_rgb( img2, d->m_image2_color_mode, img2_ocv );
  // --------------------------------------------------------------------------
  
  // Get the blended OCV image
  cv::Mat img3_ocv;
  d->blend_images( img1_ocv, img2_ocv, img3_ocv );
  vital::image_container_sptr img3( new arrows::ocv::image_container( img3_ocv ) );

  push_to_port_using_trait( blended_image, img3 );
}


// ------------------------------------------------------------------
void
blend_images_process::make_ports()
{
  // Set up for required ports
  sprokit::process::port_flags_t required;
  sprokit::process::port_flags_t optional;

  required.insert( flag_required );

  // -- input --
  declare_input_port_using_trait( image1, required );
  declare_input_port_using_trait( image2, required );

  // -- output --
  declare_output_port_using_trait( blended_image, required );
}


// ------------------------------------------------------------------
void
blend_images_process::make_config()
{
  declare_config_using_trait( blending_mode );
  declare_config_using_trait( image1_alpha );
  declare_config_using_trait( image1_mult );
  declare_config_using_trait( image1_color_mode );
  declare_config_using_trait( image2_alpha );
  declare_config_using_trait( image2_mult );
  declare_config_using_trait( image2_color_mode );
}

} //end namespace
