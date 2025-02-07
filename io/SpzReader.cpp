
#include "SpzReader.hpp"

#include <pdal/PointView.hpp>
#include <pdal/util/IStream.hpp>

#include <spz/src/cc/load-spz.h>

namespace pdal
{

static StaticPluginInfo const s_info
{
    "readers.spz",
    "SPZ Reader",
    "http://pdal.io/stages/",
    { "spz" }
};

CREATE_STATIC_STAGE(SpzReader, s_info)
std::string SpzReader::getName() const { return s_info.name; }

SpzReader::SpzReader()
{}

void SpzReader::addArgs(ProgramArgs& args)
{}

// this function would make more sense if we don't already load the entire cloud in initialize
void SpzReader::extractHeaderData()
{
    m_numPoints = m_data->numPoints;
    std::cout << "num points: " << m_numPoints << '\n';
    std::cout << "x size: " << m_data->positions.size();

    m_fractionalScale = 1.0 / (1 << m_data->fractionalBits);
    switch (m_data->shDegree)
    {
        case 0:
            m_numSh = 0;
        case 1:
            m_numSh = 9;
        case 2:
            m_numSh = 24;
        case 3:
            m_numSh = 45;
    }
}

void SpzReader::initialize()
{
    ILeStream stream(m_filename);
    if (!stream)
        throwError("Unable to open file '" + m_filename + "'");
    //!! probably should seek to end of stream differently
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
    // scales will get applied when we add points (?)
    //!!add rotations
    layout->registerDim(Id::Alpha);
    layout->registerDim(Id::Red);
    layout->registerDim(Id::Green);
    layout->registerDim(Id::Blue);

    std::string prefix = "sh" + std::to_string(m_data->shDegree) + "_";
    int counter{};
    for (int i = 0; i < m_numSh; i += 3)
    {
        Id id = layout->registerOrAssignDim(prefix + std::to_string(counter) + "_r",
            Type::Float);
        m_shDims.push_back(id);
        id = layout->registerOrAssignDim(prefix + std::to_string(counter) + "_g",
            Type::Float);
        m_shDims.push_back(id);
        id = layout->registerOrAssignDim(prefix + std::to_string(counter) + "_b",
            Type::Float);
        m_shDims.push_back(id);
        counter++;
    }
}

void SpzReader::ready(PointTableRef table)
{
    m_index = 0;
}

float SpzReader::extractPositions(size_t pos)
{
    // each X/Y/Z position gets extracted from a triplet
    int32_t fixed = m_data->positions[pos];
    fixed |= m_data->positions[pos + 1] << 8;
    fixed |= m_data->positions[pos + 2] << 16;
    fixed |= (fixed & 0x800000) ? 0xff000000 : 0;
    return static_cast<float>(fixed) * m_fractionalScale;
}

point_count_t SpzReader::read(PointViewPtr view, point_count_t count)
{
    PointId idx = view->size();

    count = (std::min)(m_numPoints - m_index, count);
    point_count_t numRead = 0;
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
        for (int i = 0; i < m_numSh; ++i)
        {
            view->setField(m_shDims[i], idx, m_data->sh[numRead + i]);
        }
        numRead++;
        idx++;
    }
    m_index += numRead;

    return numRead;
}

void SpzReader::done(PointTableRef table)
{}

bool SpzReader::processOne(PointRef& point)
{
    return false;
}

} // namespace pdal