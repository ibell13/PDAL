
#include "SpzWriter.hpp"

#include <pdal/util/OStream.hpp>

#include <arbiter/arbiter.hpp>

namespace pdal
{

static StaticPluginInfo const s_info
{
        "writers.spz",
        "SPZ writer",
        "http://pdal.io/stages/writers.spz.html",
        { "spz" }
};

CREATE_STATIC_STAGE(SpzWriter, s_info)

SpzWriter::SpzWriter() : m_cloud(new spz::GaussianCloud)
{}

std::string SpzWriter::getName() const { return s_info.name; }

void SpzWriter::addArgs(ProgramArgs& args)
{
    args.add("antialiased", "Mark the data as antialiased", m_antialiased);
}

void SpzWriter::initialize()
{
    if (Utils::isRemote(filename()))
    {
        // swap our filename for a tmp file
        std::string tmpname = Utils::tempFilename(filename());
        m_remoteFilename = filename();
        setFilename(tmpname);
    }
}

void SpzWriter::checkDimensions(PointLayoutPtr layout)
{
    // looking for our spz-specific dims.
    // we expect 3 scale and 4 rotation dimensions with PLY-style labels
    for (int i = 0; i < 4; ++i)
    {
        m_scaleDims.push_back(layout->findDim("scale_" + std::to_string(i)));
        if (i != 3)
            m_rotDims.push_back(layout->findDim("rot_" + std::to_string(i)));
    }
    m_plyAlphaDim = layout->findProprietaryDim("opacity");

    // find spherical harmonics dimensions and float RGB
    //!! these are still assumed to be in the correct order
    for (const auto& dim : layout->dimTypes())
    {
        std::string dimName = Utils::tolower(layout->dimName(dim.m_id));
        if (Utils::startsWith(dimName, "f_rest_"))
            m_shDims.push_back(dim.m_id);
        if (Utils::startsWith(dimName, "f_dc_"))
            m_plyColorDims.push_back(dim.m_id);
    }

    // check spherical harmonics dimensions
    switch (m_shDims.size())
    {
        case 0:
            m_shDegree = 0;
            break;
        case 9:
            m_shDegree = 1;
            break;
        case 24:
            m_shDegree = 2;
            break;
        case 45:
            m_shDegree = 3;
            break;
        default:
            log()->get(LogLevel::Warning) << "Invalid spherical harmonics dimensions " <<
                "for '" << filename() << "': expected 0, 9, 24 or 45 dimensions " <<
                "labeled 'f_rest_*': found " << m_shDims.size() << std::endl;
            m_shDims.clear();
            m_shDegree = 0;
    }
    // check float RGB
    if (m_plyColorDims.size() != 3)
        m_plyColorDims.clear();
}

void SpzWriter::prepared(PointTableRef table)
{
    checkDimensions(table.layout());
}

float unpackRgb(int rgb)
{
    return ((rgb / 255.0f) - 0.5f) / 0.15f;
}

float unpackAlpha(int alpha)
{
    return std::log((alpha / 255.0f) / (1 - (alpha / 255.0f)));
}

//!! not great. a bit redundant
void SpzWriter::assignRgb(const PointRef& point, size_t pos)
{
    //!! doing these checks for each point is a hassle. Better
    //to know beforehand
    if (m_plyColorDims.size())
    {
        for (int i = 0; i < 3; ++i)
            m_cloud->colors[pos + i] = point.getFieldAs<float>(m_plyColorDims[i]);
    }
    else
    {
        m_cloud->colors[pos] = unpackRgb(point.getFieldAs<int>(Dimension::Id::Red));
        m_cloud->colors[pos + 1] = unpackRgb(point.getFieldAs<int>(Dimension::Id::Green));
        m_cloud->colors[pos + 2] = unpackRgb(point.getFieldAs<int>(Dimension::Id::Blue));
    }
}

//!! messy
void SpzWriter::write(const PointViewPtr data)
{
    point_count_t pointCount = data->size();
    //!! do some check for the max size of file here?

    // from spz lib
    m_cloud->positions.resize(pointCount * 3);
    m_cloud->scales.resize(pointCount * 3);
    m_cloud->rotations.resize(pointCount * 4);
    m_cloud->alphas.resize(pointCount);
    m_cloud->colors.resize(pointCount * 3);
    m_cloud->sh.resize(pointCount * m_shDims.size());

    PointRef point(*data, 0);
    for (PointId idx = 0; idx < pointCount; ++idx)
    {
        point.setPointId(idx);
        size_t start3 = idx * 3;

        //!! combine these?
        m_cloud->positions[start3] = point.getFieldAs<float>(Dimension::Id::X);
        m_cloud->positions[start3 + 1] = point.getFieldAs<float>(Dimension::Id::Y);
        m_cloud->positions[start3 + 2] = point.getFieldAs<float>(Dimension::Id::Z);

        for (int i = 0; i < 3; ++i)
        {
            m_cloud->scales[start3 + i] = point.getFieldAs<float>(m_scaleDims[i]);
        }
        // could have 3 or 4 rotation dimensions (if W is included or not). 
        size_t start4 = idx * 4;
        for (int i = 0; i < 4; ++i)
        {
            m_cloud->rotations[start4 + i] = point.getFieldAs<float>(m_rotDims[i]);
        }

        assignRgb(point, start3);

        //!! converting from int -> float -> int, bad
        //!! both of these dims might not be there, and this would be pointless.
        if (m_plyAlphaDim != Dimension::Id::Unknown)
            m_cloud->alphas[size_t(idx)] = point.getFieldAs<float>(m_plyAlphaDim);
        else
            m_cloud->alphas[size_t(idx)] = unpackAlpha(
                point.getFieldAs<int>(Dimension::Id::Alpha));

        if (m_shDegree)
        {
            size_t shPos = idx * m_shDims.size();
            for (size_t i = 0; i < m_shDims.size(); i++) 
            {
                m_cloud->sh[shPos + i] = point.getFieldAs<float>(m_shDims[i]);
            }
        }
    }
    m_cloud->numPoints = int(pointCount);
    m_cloud->shDegree = m_shDegree;
    m_cloud->antialiased = m_antialiased;
}

void SpzWriter::done(PointTableRef table)
{
    spz::saveSpz(*m_cloud.get(), filename());

    //!! if vector<char> could play nice with vector<uint8>, I could use the other version of 
    //saveSpz & write everything w/ arbiter w/o worrying about temp files
    if (m_remoteFilename.size())
    {
        arbiter::Arbiter a;
        a.put(filename(), a.getBinary(filename()));

        FileUtils::deleteFile(filename());

        // probably don't need to do this
        setFilename(m_remoteFilename);
        m_remoteFilename.clear();
    }
}

} // namespace pdal
