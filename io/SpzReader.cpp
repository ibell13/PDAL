
#include "SpzReader.hpp"

#include <pdal/PointView.hpp>
#include <pdal/util/IStream.hpp>

#include <arbiter/arbiter.hpp>

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
{
    //!! Should give the option to write RGB/alpha as float or int;
    //or add a full spec for the dimensions.
}

void SpzReader::extractHeaderData()
{
    m_numPoints = m_data->numPoints;

    // total number of harmonics / 3
    constexpr std::array<int, 4> numHarmonics { 0, 3, 8, 15 };
    m_numSh = numHarmonics[m_data->shDegree];
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

    for (int i = 0; i < (m_numSh * 3); ++i)
        m_shDims.push_back(layout->assignDim("f_rest_" + std::to_string(i),
            Type::Float));
}

void SpzReader::ready(PointTableRef table)
{
    m_index = 0;
}

point_count_t SpzReader::read(PointViewPtr view, point_count_t count)
{
    PointId idx = view->size();

    count = (std::min)(m_numPoints - m_index, count);
    //!! make sure all the indexing is happening correctly
    point_count_t numRead = m_index;
    while (numRead < count)
    {
        //!! keeping this for now so we don't lose data on the type conversions.
        //should add some options for dim assignment.
        size_t start3 = numRead * 3;
        view->setField(Dimension::Id::Alpha, idx, m_data->alphas[numRead]);
        view->setField(Dimension::Id::Red, idx, m_data->colors[start3]);
        view->setField(Dimension::Id::Green, idx, m_data->colors[start3 + 1]);
        view->setField(Dimension::Id::Blue, idx, m_data->colors[start3 + 2]);

        spz::UnpackedGaussian unpacked = m_data->unpack(numRead);

        view->setField(Dimension::Id::X, idx, unpacked.position[0]);
        view->setField(Dimension::Id::Y, idx, unpacked.position[1]);
        view->setField(Dimension::Id::Z, idx, unpacked.position[2]);


        // rotation - xyz
        for (size_t i = 0; i < 4; ++i)
            view->setField(m_rotDims[i], idx, unpacked.rotation[i]);

        // scale
        for (size_t i = 0; i < 3; ++i)
            view->setField(m_scaleDims[i], idx, unpacked.scale[i]);

        // spherical harmonics
        for (size_t i = 0; i < m_numSh; i)
        {
            size_t pos = i * 3;
            view->setField(m_shDims[pos], idx, unpacked.shR[i]);
            view->setField(m_shDims[pos + 1], idx, unpacked.shG[i]);
            view->setField(m_shDims[pos + 2], idx, unpacked.shB[i]);
        }

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