
#pragma once

#include <pdal/Reader.hpp>
#include <pdal/Dimension.hpp>

namespace spz
{
    struct PackedGaussians;
}
namespace pdal
{

class PDAL_EXPORT SpzReader : public Reader
{
public:
    SpzReader();

    std::string getName() const;
private:
    point_count_t m_numPoints;
    point_count_t m_index;
    int m_numSh;
    float m_fractionalScale;
    //!! put these in a struct or something?
    Dimension::IdList m_shDims;
    Dimension::IdList m_rotDims;
    Dimension::IdList m_scaleDims;
    std::unique_ptr<spz::PackedGaussians> m_data;

    virtual void addArgs(ProgramArgs& args);
    virtual void initialize();
    virtual void addDimensions(PointLayoutPtr layout);
    virtual void ready(PointTableRef table);
    virtual point_count_t read(PointViewPtr view, point_count_t num);
    virtual void done(PointTableRef table);

    void extractHeaderData();
    double extractPositions(size_t pos);
    float unpackSh(size_t pos);
    float unpackScale(size_t pos);
};

} // namespace pdal