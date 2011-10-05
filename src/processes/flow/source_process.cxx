/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "source_process.h"

#include <vistk/pipeline/datum.h>
#include <vistk/pipeline/edge.h>
#include <vistk/pipeline/stamp.h>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

/**
 * \file source_process.cxx
 *
 * \brief Implementation of the source process.
 */

namespace vistk
{

class source_process::priv
{
  public:
    priv();
    ~priv();

    stamp_t color_stamp;
    edge_group_t edges;

    static port_t const port_output;
};

process::port_t const source_process::priv::port_output = process::port_t("color");

source_process
::source_process(config_t const& config)
  : process(config)
  , d(new priv)
{
  port_flags_t required;

  required.insert(flag_required);

  declare_output_port(priv::port_output, boost::make_shared<port_info>(
    type_none,
    required,
    port_description_t("A consistently colored output stamp.")));
}

source_process
::~source_process()
{
}

void
source_process
::_init()
{
  d->edges = output_port_edges(priv::port_output);
}

void
source_process
::_step()
{
  datum_t const dat = datum::empty_datum();

  edge_datum_t const edat = edge_datum_t(dat, d->color_stamp);

  bool has_incomplete = false;
  bool needs_data = false;

  /// \todo Replace this busy loop.
  do
  {
    has_incomplete = false;

    BOOST_FOREACH (edge_ref_t& edge_ref, d->edges)
    {
      edge_t const edge = edge_ref.lock();

      if (!edge->is_downstream_complete())
      {
        has_incomplete = true;

        if (!edge->has_data())
        {
          needs_data = true;
        }
      }
    }
  } while (has_incomplete && !needs_data);

  push_to_port(priv::port_output, edat);

  d->color_stamp = stamp::incremented_stamp(d->color_stamp);

  process::_step();
}

source_process::priv
::priv()
  : color_stamp(stamp::new_stamp())
{
}

source_process::priv
::~priv()
{
}

}
