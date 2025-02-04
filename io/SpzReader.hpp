
#pragma once

#include <pdal/Reader.hpp>
#include <pdal/Dimension.hpp>
#include <pdal/Streamable.hpp>

namespace pdal
{

class PDAL_EXPORT SpzReader : public Reader, public Streamable
{
public:
    SpzReader();

    std::string getName() const;
private:
    point_count_t m_numPoints;
    std::size_t m_offset;

    virtual void addArgs(ProgramArgs& args);
    virtual void initialize();
    virtual void addDimensions(PointLayoutPtr layout);
    virtual void ready(PointTableRef table);
    virtual point_count_t read(PointViewPtr view, point_count_t num);
    virtual void done(PointTableRef table);
    virtual bool processOne(PointRef& point);

    void extractHeader();
};

} // namespace pdal