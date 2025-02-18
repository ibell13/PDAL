
#include "SpzWriter.hpp"

#include <pdal/util/OStream.hpp>

#include <spz/src/cc/load-spz.h>

namespace pdal
{

static StaticPluginInfo const s_info
{
        "writers.spz",
        "SPZ writer",
        "http://pdal.io/stages/",
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
    // add remote file stuff, maybe?
}

void SpzWriter::checkDimensions(PointLayoutPtr layout)
{
    m_dims = layout->dimTypes();
    for (const auto& dim : m_dims)
    {
        // looking for our spz-specific dims.
        //!! Figure out what happens if we don't find them
        //!! Not good to bank on all of these being in order. maybe sort the
        // IdLists or do this another way.
        std::string dimName = Utils::tolower(layout->dimName(dim.m_id));
        if (Utils::startsWith(dimName, "f_rest_"))
            m_shDims.push_back(dim.m_id);
        else if (Utils::startsWith(dimName, "scale_"))
            m_scaleDims.push_back(dim.m_id);
        else if (Utils::startsWith(dimName, "rot_"))
            m_rotDims.push_back(dim.m_id);
    }

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
}

void SpzWriter::prepared(PointTableRef table)
{
    checkDimensions(table.layout());
}

//!! maybe put this into a lambda in write()?
float SpzWriter::tryGetDim(const PointRef& point, Dimension::Id id)
{
    float f;
    try
    {
        f = point.getFieldAs<float>(id);
    }
    catch(std::exception&)
    {
        return 0.0f;
    }

    return f;
}

float unpackRgb(const PointRef& point, Dimension::Id id)
{
    try
    {
        
    }
    catch(std::exception&)
    {
        return 0;
    }
    
    return ((point.getFieldAs<uint8_t>(id) / 255.0f) - 0.5f) / 0.15f;
}

float unpackAlpha(uint8_t alpha)
{
    return std::log((alpha / 255.0f) / (1 - (alpha / 255.0f)));
}

//!! messy
void SpzWriter::write(const PointViewPtr data)
{
    point_count_t pointCount = data->size();
    //!! do some check for the max size of file here? pointCount * ~64?

    //!! from spz lib
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

        //!! this is assuming we have the correct order of scale & rotation dimensions. need to verify earlier
        for (int i = 0; i < 3; ++i)
        {
            m_cloud->scales[start3 + i] = tryGetDim(point, m_scaleDims[i]);
        }
        //!! could have 3 or 4 rotation dimensions (if W is included or not). Figure out what happens if not
        size_t start4 = idx * 4;
        for (int i = 0; i < 4; ++i)
        {
            m_cloud->rotations[start4 + i] = tryGetDim(point, m_rotDims[i]);
        }
        //!! again, this is assuming RGB & alpha are present. fix.
        //!! maybe to take into account PLY RGB dimensions ('f_dc_*' & 'opacity') - these could be float
        //and wouldn't need to be unpacked from int.
        //!! converting from int -> float -> int, bad (for alpha especially)
        m_cloud->alphas[size_t(idx)] = unpackAlpha(point.getFieldAs<uint8_t>(Dimension::Id::Alpha));
        m_cloud->colors[start3] = unpackRgb(point, Dimension::Id::Red);
        m_cloud->colors[start3 + 1] = unpackRgb(point, Dimension::Id::Green);
        m_cloud->colors[start3 + 2] = unpackRgb(point, Dimension::Id::Blue);

        if (m_shDegree)
        {
            size_t shPos = idx * m_shDims.size();
            for (size_t i = 0; i < m_shDims.size(); i++) 
            {
                m_cloud->sh[shPos + i] = tryGetDim(point, m_shDims[i]);
            }
        }
    }
    m_cloud->numPoints = pointCount;
    m_cloud->shDegree = m_shDegree;
    m_cloud->antialiased = m_antialiased;
}

void SpzWriter::done(PointTableRef table)
{
    //!! use method that writes to binary instead
    spz::saveSpz(*m_cloud.get(), filename());
}

} // namespace pdal
