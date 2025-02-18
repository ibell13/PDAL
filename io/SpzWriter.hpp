
#pragma once

#include <pdal/PointView.hpp>
#include <pdal/Writer.hpp>

namespace spz
{
    struct GaussianCloud;
}

namespace pdal
{

/* struct SpzDimensions
{
    Dimension::IdList shDims;
    Dimension::IdList rotDims;
    Dimension::IdList scaleDims;
    Dimension::IdList colorDims;
    Dimension::Id opacity;
};
 */
class PDAL_EXPORT SpzWriter : public Writer
{
public:
    // spz lib always uses 12 fractional bits currently
    SpzWriter();
    std::string getName() const;

private:
    virtual void addArgs(ProgramArgs& args);
    virtual void initialize();
    virtual void prepared(PointTableRef table);
    //virtual void readyFile(const std::string& filename, const SpatialReference& srs);
    virtual void write(const PointViewPtr view);
    virtual void done(PointTableRef table);

    void checkDimensions(PointLayoutPtr layout);
    float tryGetDim(const PointRef& point, Dimension::Id id);
    void writeRgb(const PointRef& point, size_t pos);

    bool m_antialiased;
    //!! OLeStream?
    std::ostream *m_stream;
    DimTypeList m_dims;
    std::unique_ptr<spz::GaussianCloud> m_cloud;
    //!! again, maybe keep these grouped together
    Dimension::IdList m_shDims;
    Dimension::IdList m_rotDims;
    Dimension::IdList m_scaleDims;
    //Dimension::IdList m_colorDims;
    int m_shDegree;
    float m_fractionalScale;
    std::vector<PointViewPtr> m_views;
    PointLayoutPtr m_layout;
    std::string m_curFilename;
};

} // namespace pdal
