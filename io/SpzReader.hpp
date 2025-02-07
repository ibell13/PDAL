
#pragma once

#include <pdal/Reader.hpp>
#include <pdal/Dimension.hpp>
#include <pdal/Streamable.hpp>

namespace spz
{
    struct PackedGaussians;
}
namespace pdal
{

class PDAL_EXPORT SpzReader : public Reader, public Streamable
{
public:
    SpzReader();

    std::string getName() const;
private:
    point_count_t m_numPoints;
    point_count_t m_index;
    //std::size_t m_offset;
    int m_numSh;
    float m_fractionalScale;
    Dimension::IdList m_shDims;
    //GaussianCloudData m_headerData;
    //std::unique_ptr<std::vector<uint8_t>> m_data;
    std::unique_ptr<spz::PackedGaussians> m_data;

    virtual void addArgs(ProgramArgs& args);
    virtual void initialize();
    virtual void addDimensions(PointLayoutPtr layout);
    virtual void ready(PointTableRef table);
    virtual point_count_t read(PointViewPtr view, point_count_t num);
    virtual void done(PointTableRef table);
    virtual bool processOne(PointRef& point);

    void extractHeaderData();
    float extractPositions(size_t pos);
};

} // namespace pdal