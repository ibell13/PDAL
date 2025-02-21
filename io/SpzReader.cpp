
#include "SpzReader.hpp"

#include <pdal/PointView.hpp>
#include <pdal/util/IStream.hpp>

#include <arbiter/arbiter.hpp>
#include <spz/src/cc/load-spz.h>

namespace pdal
{

static StaticPluginInfo const s_info
{
    "readers.spz",
    "SPZ Reader",
    "http://pdal.io/stages/readers.spz.html",
    { "spz" }
};

CREATE_STATIC_STAGE(SpzReader, s_info)
std::string SpzReader::getName() const { return s_info.name; }

SpzReader::SpzReader()
{}

void SpzReader::addArgs(ProgramArgs& args)
{}

void SpzReader::extractHeaderData()
{
    m_numPoints = m_data->numPoints;
    m_fractionalScale = 1.0 / (1 << m_data->fractionalBits);

    switch (m_data->shDegree)
    {
        case 0:
            m_numSh = 0;
            break;
        case 1:
            m_numSh = 9;
            break;
        case 2:
            m_numSh = 24;
            break;
        case 3:
            m_numSh = 45;
            break;
    }
}

void SpzReader::initialize()
{
    // copy the file to local if it's remote
    m_isRemote = Utils::isRemote(m_filename);
    if (m_isRemote)
    {
        std::string remoteFilename = Utils::tempFilename(m_filename);
        // swap m_filename to temp file, remoteFilename to original
        std::swap(remoteFilename, m_filename);

        arbiter::Arbiter a;
        a.put(m_filename, a.getBinary(remoteFilename));
    }

    ILeStream stream(m_filename);
    if (!stream)
        throwError("Unable to open file '" + m_filename + "'");
    stream.seek(0, std::ios::end);
    std::vector<uint8_t> data(stream.position());
    stream.seek(0, std::ios::beg);
    stream.get(reinterpret_cast<char *>(data.data()), data.size());
    stream.close();
    //!! spz lib does a .good() check here but it breaks stuff for me. 
    //need to figure out what's going on

    m_data.reset(new spz::PackedGaussians(spz::loadSpzPacked(data)));
    if (m_data->usesFloat16())
        throwError("SPZ float16 point encoding not supported!");

    extractHeaderData();
}

void SpzReader::addDimensions(PointLayoutPtr layout)
{
    using namespace Dimension;

    layout->registerDim(Id::X);
    layout->registerDim(Id::Y);
    layout->registerDim(Id::Z);
    layout->registerDim(Id::Alpha);
    layout->registerDim(Id::Red);
    layout->registerDim(Id::Green);
    layout->registerDim(Id::Blue);

    // scale/rotation/SH dimensions set with PLY naming conventions
    for (int i = 0; i < 3; ++i)
        m_scaleDims.push_back(layout->assignDim("scale_" + std::to_string(i),
            Type::Float));

    for (int i = 0; i < 4; ++i)
        m_rotDims.push_back(layout->assignDim("rot_" + std::to_string(i),
            Type::Float));

    for (int i = 0; i < m_numSh; ++i)
        m_shDims.push_back(layout->assignDim("f_rest_" + std::to_string(i),
            Type::Float));
}

void SpzReader::ready(PointTableRef table)
{
    m_index = 0;
}

// each X/Y/Z position gets extracted from a triplet
double SpzReader::extractPositions(size_t pos)
{
    // Decode 24-bit fixed point coordinates
    int32_t fixed = m_data->positions[pos];
    fixed |= m_data->positions[pos + 1] << 8;
    fixed |= m_data->positions[pos + 2] << 16;
    fixed |= (fixed & 0x800000) ? 0xff000000 : 0; // sign extension to 32 bits
    return static_cast<double>(fixed) * m_fractionalScale; 
}

float SpzReader::unpackSh(size_t pos)
{
    return (static_cast<float>(m_data->sh[pos]) - 128.0f) / 128.0f;
}

float SpzReader::unpackScale(size_t pos)
{
    return m_data->scales[pos] / 16.0f - 10.0f;
}

point_count_t SpzReader::read(PointViewPtr view, point_count_t count)
{
    PointId idx = view->size();

    count = (std::min)(m_numPoints - m_index, count);
    //!! make sure all the indexing is happening correctly
    point_count_t numRead = m_index;
    PointRef point = PointRef(*view, 0);
    while (numRead < count)
    {
        size_t start3 = numRead * 3;
        size_t xPos = start3 * 3;

        view->setField(Dimension::Id::X, idx, extractPositions(xPos));
        view->setField(Dimension::Id::Y, idx, extractPositions(xPos + 3));
        view->setField(Dimension::Id::Z, idx, extractPositions(xPos + 6));
        view->setField(Dimension::Id::Alpha, idx, m_data->alphas[numRead]);
        view->setField(Dimension::Id::Red, idx, m_data->colors[start3]);
        view->setField(Dimension::Id::Green, idx, m_data->colors[start3 + 1]);
        view->setField(Dimension::Id::Blue, idx, m_data->colors[start3 + 2]);

        // rotation - xyz
        std::vector<float> xyzSquared;
        for (int i = 0; i < 3; ++i)
        {
            float rotation = (m_data->rotations[start3 + i] / 127.5f) - 1;
            view->setField(m_rotDims[i], idx, rotation);
            xyzSquared.push_back(rotation * rotation);
        }
        // rotation - w
        float squaredNorm = xyzSquared[0] + xyzSquared[1] + xyzSquared[2];
        view->setField(m_rotDims[3], idx,
            std::sqrt((std::max)(0.0f, 1.0f - squaredNorm)));

        // scale
        for (int i = 0; i < 3; ++i)
            view->setField(m_scaleDims[i], idx, unpackScale(start3 + i));

        // spherical harmonics
        size_t shPos = numRead * m_numSh;
        for (int i = 0; i < m_numSh; ++i)
            view->setField(m_shDims[i], idx, unpackSh(shPos + i));

        numRead++;
        idx++;
    }
    m_index += numRead;

    return numRead;
}

void SpzReader::done(PointTableRef table)
{
    // delete tmp file
    if (m_isRemote)
        FileUtils::deleteFile(m_filename);
}

} // namespace pdal