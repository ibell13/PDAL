#pragma once

#include <pdal/Filter.hpp>
#include <pdal/Streamable.hpp>
#include <pdal/util/ProgramArgs.hpp>

namespace hexer
{
    class HexGrid;
};

namespace pdal
{

class HexBoundary : public Filter, public Streamable
{
public:
    HexBoundary();
    ~HexBoundary();

    HexBoundary& operator=(const HexBoundary&) = delete;
    HexBoundary(const HexBoundary&) = delete;

    std::string getName() const;
private:
    std::unique_ptr<hexer::HexGrid> m_grid;
    uint32_t m_precision;
    uint32_t m_sampleSize;
    double m_cullArea;
    Arg *m_cullArg;
    int32_t m_density;
    double m_edgeLength;
    bool m_doSmooth;
    point_count_t m_count;
    bool m_preserve_topology;
    SpatialReference m_srs;

    virtual void addArgs(ProgramArgs& args);
    virtual void ready(PointTableRef table);
    virtual void filter(PointView& view);
    virtual bool processOne(PointRef& point);
    virtual void spatialReferenceChanged(const SpatialReference& srs);
    virtual void done(PointTableRef table);
};

} // namespace pdal