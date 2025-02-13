
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

std::string SpzWriter::getName() const { return s_info.name; }

void SpzWriter::addArgs(ProgramArgs& args)
{
    // maybe add fractional bits or antialiased options?
}

void SpzWriter::initialize()
{
    // add remote file stuff, maybe?

    // need to reset these for each point table, I think?
    //there's probably a better way
    m_shDims = {};
    m_scaleDims = {};
    m_rotDims = {};
}

void SpzWriter::checkDimensions(PointLayoutPtr layout)
{
    m_dims = layout->dimTypes();
    //std::sort(m_dims.begin(), m_dims.end());
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
    //!! allow more leeway here?
    switch (m_shDims.size())
    {
        case 0:
            m_shDegree = 0;
        case 9:
            m_shDegree = 1;
        case 24:
            m_shDegree = 2;
        case 45:
            m_shDegree = 3;
        default:
            log()->get(LogLevel::Warning) << "Invalid spherical harmonics dimensions " <<
                "for '" << Writer::filename() << "': expected 0, 9, 24 or 45 dimensions " <<
                "labeled 'f_rest_*': found " << m_shDims.size() << std::endl;
            m_shDims.clear();
            m_shDegree = 0;
    }
}

void SpzWriter::prepared(PointTableRef table)
{
    FlexWriter::validateFilename(table);

    checkDimensions(table.layout());
}

//!! took these from PlyReader. Maybe should touch them up
void SpzWriter::readyTable(PointTableRef table)
{
    m_layout = table.layout();
}

void SpzWriter::doneTable(PointTableRef table)
{
    m_layout = nullptr;
}

void SpzWriter::readyFile(const std::string& filename, const SpatialReference& srs)
{
    m_curFilename = filename;
    Utils::writeProgress(m_progressFd, "READYFILE", filename);
}

void SpzWriter::writePoint(spz::PackedGaussians& cloud)
{}

// copied from spz lib
uint8_t toUint8(float x)
{
    return static_cast<uint8_t>(std::clamp(std::round(x), 0.0f, 255.0f));
}

uint8_t quantizeSH(float x, int bucketSize)
{
    int q = static_cast<int>(std::round(x * 128.0f) + 128.0f);
    q = (q + bucketSize / 2) / bucketSize * bucketSize;
    return static_cast<uint8_t>(std::clamp(q, 0, 255));
}

size_t countBytes(std::vector<uint8_t> vec)
{
    return vec.size() * sizeof(vec[0]);
}

int32_t SpzWriter::castPosition(float xyz)
{
    return static_cast<int32_t>(std::round(xyz * m_fractionalScale));
}

void SpzWriter::assignPositions(spz::PackedGaussians& cloud, int32_t point, size_t pos)
{
    cloud.positions[pos] = point & 0xff;
    cloud.positions[pos + 1] = (point >> 8) & 0xff;
    cloud.positions[pos + 2] = (point >> 16) & 0xff;
}

//!! messy
void SpzWriter::writeView(const PointViewPtr data)
{
    point_count_t pointCount = data->size();
    //!! do some check for the max size of file here? pointCount * ~64?

    //!! if this is a GaussianCloud we can use the spz saveSPZ() for writing,
    //which might be helpful - don't have to worry about any Zlib/Gzip stuff.
    //but saveSPZ() converts everything back to PackedGaussians, maybe losing
    //data in the conversion from RGB/Alpha int->float->int. 
    spz::PackedGaussians packed;
    //!! from spz lib
    packed.positions.resize(pointCount * 9);
    packed.scales.resize(pointCount * 3);
    packed.rotations.resize(pointCount * 3);
    packed.alphas.resize(pointCount);
    packed.colors.resize(pointCount * 3);
    packed.sh.resize(pointCount * m_shDims.size() * 3);

    m_fractionalScale = (1 << m_fractionalBits);
    PointRef point(*data, 0);
    //!! put more of this loop inside functions - everything that extracts a dimension
    //should have a default value to write if the dimension isn't present.
    //!! probably possible to put dimensions directly into the stream, but the ordering
    //needs each full dim to be extracted from the PointView individually (see load-spz.cc:442)
    for (PointId idx = 0; idx < pointCount; ++idx)
    {
        point.setPointId(idx);
        size_t start3 = idx * 3;
        size_t xPos = start3 * 3;

        //!! combine these.
        assignPositions(packed, castPosition(point.getFieldAs<float>(Dimension::Id::X)), xPos);
        assignPositions(packed, castPosition(point.getFieldAs<float>(Dimension::Id::Y)), xPos + 3);
        assignPositions(packed, castPosition(point.getFieldAs<float>(Dimension::Id::Z)), xPos + 6);

        //!! this is assuming we have the correct scale & rotation dimensions. need to verify earlier
        for (int i = 0; i < 3; ++i)
        {
            packed.scales[start3 + i] = toUint8((point.getFieldAs<float>(m_scaleDims[i]) + 10.0f) * 16.0f);
            //!! spz lib does a lot more normalization of the rotations when packing.
            //I probably need to also, but I'm leaving it like this for now.
            packed.rotations[start3 + i] = toUint8(127.5f * (point.getFieldAs<float>(m_scaleDims[i]) + 1.0f));
        }
        //!! again, this is assuming RGB & alpha are present. fix.
        //!! maybe to take into account PLY RGB dimensions ('f_dc_*' & 'opacity')
        packed.alphas[size_t(idx)] = point.getFieldAs<uint8_t>(Dimension::Id::Alpha);
        packed.colors[start3] = point.getFieldAs<uint8_t>(Dimension::Id::Red);
        packed.colors[start3 + 1] = point.getFieldAs<uint8_t>(Dimension::Id::Green);
        packed.colors[start3 + 2] = point.getFieldAs<uint8_t>(Dimension::Id::Blue);

        if (m_shDegree)
        {
            // from spz library
            constexpr int sh1Bits = 5;
            constexpr int shRestBits = 4;
            size_t j = m_shDims.size();
            size_t shPos = idx * j;
            // we can always assume at least 9 SH values (degree 1)
            for (; j < 9; j++) 
            {
                packed.sh[shPos + j] = quantizeSH(point.getFieldAs<float>(m_shDims[j]),
                                        1 << (8 - sh1Bits));
            }
            for (; j < m_shDims.size(); j++) 
            {
                packed.sh[shPos + j] = quantizeSH(point.getFieldAs<float>(m_shDims[j]),
                                        1 << (8 - shRestBits));
            }
        }
    }
    spz::PackedGaussiansHeader header;
    header.numPoints = static_cast<uint32_t>(pointCount);
    header.shDegree = static_cast<uint8_t>(m_shDegree);
    header.fractionalBits = static_cast<uint8_t>(m_fractionalBits);
    //!! we don't set the antialiased flag in the header. Think about how to deal with it

    m_stream->write(reinterpret_cast<const char *>(&header), sizeof(header));
    m_stream->write(reinterpret_cast<const char *>(packed.positions.data()), countBytes(packed.positions));
    m_stream->write(reinterpret_cast<const char *>(packed.alphas.data()), countBytes(packed.alphas));
    m_stream->write(reinterpret_cast<const char *>(packed.colors.data()), countBytes(packed.colors));
    m_stream->write(reinterpret_cast<const char *>(packed.scales.data()), countBytes(packed.scales));
    m_stream->write(reinterpret_cast<const char *>(packed.rotations.data()), countBytes(packed.rotations));
    m_stream->write(reinterpret_cast<const char *>(packed.sh.data()), countBytes(packed.sh));
    //!! multiple file outputs could have different header info. Have something to check against,
    //maybe in doneFile? Or give up on FlexWriter?
}

//!! PlyWriter writes everything here. Not sure if I should too
void SpzWriter::doneFile()
{

}

} // namespace pdal
