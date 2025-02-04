
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

CREATE_SHARED_STAGE(SpzReader, s_info)
std::string SpzReader::getName() const { return s_info.name; }

SpzReader::SpzReader()
{}

void SpzReader::addArgs(ProgramArgs& args)
{}

void SpzReader::initialize()
{

}

void SpzReader::addDimensions(PointLayoutPtr layout)
{

}

void SpzReader::ready(PointTableRef table)
{}

point_count_t SpzReader::read(PointViewPtr view, point_count_t num)
{}

void SpzReader::done(PointTableRef table)
{}

bool SpzReader::processOne(PointRef& point)
{}

} // namespace pdal