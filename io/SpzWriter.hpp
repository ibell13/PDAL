
#pragma once

#include <pdal/PointView.hpp>
#include <pdal/FlexWriter.hpp>

namespace spz
{
    struct PackedGaussians;

    // from spz lib. Maybe just expose this in their header
    struct PackedGaussiansHeader 
    {
        uint32_t magic = 0x5053474e;  // NGSP = Niantic gaussian splat
        uint32_t version = 2;
        uint32_t numPoints = 0;
        uint8_t shDegree = 0;
        uint8_t fractionalBits = 0;
        uint8_t flags = 0;
        uint8_t reserved = 0;
    };
}

namespace pdal
{

//!! base writer or flexwriter?
class PDAL_EXPORT SpzWriter : public FlexWriter
{
public:
    // spz lib always uses 12 fractional bits currently
    SpzWriter() : m_fractionalBits(12)
    {}
    std::string getName() const;

private:
    virtual void addArgs(ProgramArgs& args);
    virtual void initialize();
    virtual void prepared(PointTableRef table);
    virtual void readyTable(PointTableRef table);
    virtual void doneTable(PointTableRef table);
    virtual void readyFile(const std::string& filename, const SpatialReference& srs);
    virtual void writeView(const PointViewPtr data);
    virtual void doneFile();

    void checkDimensions(PointLayoutPtr layout);
    void writePoint(spz::PackedGaussians& gaussians);
    int32_t castPosition(float xyz);
    void assignPositions(spz::PackedGaussians& cloud, int32_t point, size_t pos);

    //!! OLeStream?
    std::ostream *m_stream;
    DimTypeList m_dims;
    //spz::PackedGaussiansHeader m_header;
    //!! again, maybe keep these grouped together
    Dimension::IdList m_shDims;
    Dimension::IdList m_rotDims;
    Dimension::IdList m_scaleDims;
    int m_shDegree;
    int m_fractionalBits;
    float m_fractionalScale;
    std::vector<PointViewPtr> m_views;
    PointLayoutPtr m_layout;
    std::string m_curFilename;
};

} // namespace pdal
