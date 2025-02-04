
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

void SpzReader::initialize()
{
    ILeStream stream(m_filename);
    if (!stream)
        throwError("Unable to open file '" + m_filename + "'");
    m_data.reset(new std::vector<uint8_t>(stream.position()));
    stream.seek(0, std::ios::beg);
    stream.get(reinterpret_cast<char *>(m_data->data()), m_data->size());
    stream.close();
    spz::loadSpz(*m_data.get());
}

void SpzReader::addDimensions(PointLayoutPtr layout)
{

}

void SpzReader::ready(PointTableRef table)
{}

point_count_t SpzReader::read(PointViewPtr view, point_count_t num)
{
    return 0;
}

void SpzReader::done(PointTableRef table)
{}

bool SpzReader::processOne(PointRef& point)
{
    return false;
}

} // namespace pdal