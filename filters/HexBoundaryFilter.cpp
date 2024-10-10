#include "HexBoundaryFilter.hpp"

#include "private/hexer/HexGrid.hpp"

#include <pdal/Polygon.hpp>

using namespace hexer;

namespace pdal
{

static PluginInfo const s_info = PluginInfo(
    "filters.hex_boundary",
    "Simplified version of filters.hexbin that outputs non-H3 WKT boundaries.",
    "http://pdal.io/stages/filters.hexbin.html" );

CREATE_STATIC_STAGE(HexBoundary, s_info)


HexBoundary::HexBoundary()
{}


HexBoundary::~HexBoundary()
{}


std::string HexBoundary::getName() const
{
    return s_info.name;
}


void HexBoundary::addArgs(ProgramArgs& args)
{
    args.add("sample_size", "Sample size for auto-edge length calculation",
        m_sampleSize, 5000U);
    args.add("threshold", "Required cell density", m_density, 15);
    args.add("edge_length", "Length of hex edge", m_edgeLength);
    args.add("precision", "Output precision", m_precision, 8U);
    m_cullArg = &args.add("hole_cull_area_tolerance", "Tolerance area to "
        "apply to holes before cull", m_cullArea);
    args.add("smooth", "Smooth boundary output", m_doSmooth, true);
    args.add("preserve_topology", "Preserve topology when smoothing",
        m_preserve_topology, true);
}


void HexBoundary::ready(PointTableRef table)
{
    if (m_edgeLength == 0.0)
    {
        m_grid.reset(new HexGrid(m_density));
        m_grid->setSampleSize(m_sampleSize);
    }
    else
        m_grid.reset(new HexGrid(m_edgeLength * sqrt(3), m_density));
}


void HexBoundary::filter(PointView& view)
{
    PointRef p(view, 0);

    for (PointId idx = 0; idx < view.size(); ++idx)
    {
        p.setPointId(idx);
        processOne(p);
    }
}


bool HexBoundary::processOne(PointRef& point)
{
    double x = point.getFieldAs<double>(Dimension::Id::X);
    double y = point.getFieldAs<double>(Dimension::Id::Y);
    m_grid->addXY(x, y);
    m_count++;
    return true;
}


void HexBoundary::spatialReferenceChanged(const SpatialReference& srs)
{
    m_srs = srs;
}


void HexBoundary::done(PointTableRef table)
{
    try
    {
        m_grid->findShapes();
        m_grid->findParentPaths();
    }
    catch (hexer::hexer_error& e)
    {
        m_metadata.add("error", e.what(),
            "Hexer threw an error and was unable to compute a boundary");
        m_metadata.add("boundary", "MULTIPOLYGON EMPTY",
            "Empty polygon -- unable to compute boundary");
        return;
    }

    std::ostringstream polygon;
    polygon.setf(std::ios_base::fixed, std::ios_base::floatfield);
    // is setting the precision twice necessary?
    polygon.precision(m_precision);
    m_grid->toWKT(polygon);

    Polygon p(polygon.str(), m_srs); 

    /***
    We want to make these bumps on edges go away, which means that
    we want to elimnate both B and C.  If we take a line from A -> C,
    we need the tolerance to eliminate B.  After that we're left with
    the triangle ACD and we want to eliminate C.  The perpendicular
    distance from AD to C is the hexagon height / 2, so we set the
    tolerance a little larger than that.  This is larger than the
    perpendicular distance needed to eliminate B in ABC, so should
    serve for both cases.

      B ______  C
       /      \
    A /        \ D

    ***/
    if (m_doSmooth)
    {
        double tolerance = 1.1 * m_grid->height() / 2;
        double cull = m_cullArg->set() ?
            m_cullArea : (6 * tolerance * tolerance);
        p.simplify(tolerance, cull, m_preserve_topology);
    }

    m_metadata.add("boundary", p.wkt(m_precision),
        "Approximated MULTIPOLYGON of domain");
    m_metadata.add("srs", m_srs.getWKT(),
        "Spatial reference of hexbin boundary");
}
} // namespace pdal