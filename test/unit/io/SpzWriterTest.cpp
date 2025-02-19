
#include <pdal/pdal_test_main.hpp>

#include "Support.hpp"

#include <io/BufferReader.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/PipelineManager.hpp>

#include <io/SpzReader.hpp>
#include <io/SpzWriter.hpp>

using namespace pdal;

PointViewPtr setXYZ(PointTableRef table)
{
    table->layout()->registerDims(Dimension::Id::X);
    table->layout()->registerDims(Dimension::Id::Y);
    table->layout()->registerDims(Dimension::Id::Z);

    PointViewPtr v(new PointView(table));
    v->setField(Dimension::Id::X, 0, 1);
    v->setField(Dimension::Id::Y, 0, 1);
    v->setField(Dimension::Id::Z, 0, 0);

    v->setField(Dimension::Id::X, 1, 2);
    v->setField(Dimension::Id::Y, 1, 1);
    v->setField(Dimension::Id::Z, 1, 0);

    v->setField(Dimension::Id::X, 2, 1);
    v->setField(Dimension::Id::Y, 2, 2);
    v->setField(Dimension::Id::Z, 2, 0);

    return v;
}

TEST(SpzWriterTest, all_dimensions_test)
{
    

    BufferReader r;

    SpzWriter writer;
    opts.replace("filename", Support::datapath("test.spz"));
    writer.setInput(reader);
    writer.setOptions(opts);

    std::cout << "point count : " << mgr.execute();
}

// add test for invalid dimensions, re-ordered dimensions, etc
