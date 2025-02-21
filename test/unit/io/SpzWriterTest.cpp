
#include <pdal/pdal_test_main.hpp>

#include "Support.hpp"

#include <io/BufferReader.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/PipelineManager.hpp>
#include <pdal/util/FileUtils.hpp>

#include <io/SpzWriter.hpp>

using namespace pdal;

PointViewPtr setXYZ(PointTableRef table)
{
    table.layout()->registerDim(Dimension::Id::X);
    table.layout()->registerDim(Dimension::Id::Y);
    table.layout()->registerDim(Dimension::Id::Z);

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

PointViewPtr setAllDims(PointTableRef table)
{
    // registering dims
    table.layout()->registerDim(Dimension::Id::Alpha);
    table.layout()->registerDim(Dimension::Id::Red);
    table.layout()->registerDim(Dimension::Id::Green);
    table.layout()->registerDim(Dimension::Id::Blue);

    Dimension::IdList scaleIds;
    for (int i = 0; i < 3; ++i)
        scaleIds.push_back(table.layout()->assignDim("scale_" + std::to_string(i),
            Dimension::Type::Float));
    Dimension::IdList rotIds;
    for (int i = 0; i < 4; ++i)
        rotIds.push_back(table.layout()->assignDim("rot_" + std::to_string(i),
            Dimension::Type::Float));
    Dimension::IdList shIds;
    for (int i = 0; i < 9; ++i)
        shIds.push_back(table.layout()->assignDim("f_rest_" + std::to_string(i),
            Dimension::Type::Float));

    PointViewPtr v = setXYZ(table);
    // setting dims
    v->setField(Dimension::Id::Alpha, 0, 10);
    v->setField(Dimension::Id::Red, 0, 10);
    v->setField(Dimension::Id::Green, 0, 10);
    v->setField(Dimension::Id::Blue, 0, 10);

    v->setField(Dimension::Id::Alpha, 1, 20);
    v->setField(Dimension::Id::Red, 1, 20);
    v->setField(Dimension::Id::Green, 1, 20);
    v->setField(Dimension::Id::Blue, 1, 20);
    
    v->setField(Dimension::Id::Alpha, 2, 30);
    v->setField(Dimension::Id::Red, 2, 30);
    v->setField(Dimension::Id::Green, 2, 30);
    v->setField(Dimension::Id::Blue, 2, 30);

    for (int i = 0; i < 3; i++)
    {
        for (auto s : scaleIds)
            v->setField(s, i, (0.1f * (i + 1)));
        for (auto r : rotIds)
            v->setField(r, i, (0.2f * (i + 1)));
        for (auto sh : shIds)
            v->setField(sh, i, (0.3f * (i + 1)));
    }   
    return v;
}

TEST(SpzWriterTest, xyz_only_test)
{
    BufferReader r;
    PointTable t;
    PointViewPtr v = setXYZ(t);
    r.addView(v);

    SpzWriter writer;
    Options opts;
    std::string path = Support::temppath("out.spz");
    opts.replace("filename", path);
    writer.setInput(r);
    writer.setOptions(opts);

    writer.prepare(t);
    writer.execute(t);
    ASSERT_EQ(FileUtils::fileSize(path), 49);
}

TEST(SpzWriterTest, all_dimensions_test)
{
    PointTable t;
    BufferReader r;
    PointViewPtr v = setAllDims(t);
    r.addView(v);

    SpzWriter writer;
    Options opts;
    std::string path = Support::temppath("out.spz");
    opts.replace("filename", path);
    writer.setInput(r);
    writer.setOptions(opts);

    writer.prepare(t);
    writer.execute(t);
    ASSERT_EQ(FileUtils::fileSize(path), 78);
}

// add test for invalid dimensions, re-ordered dimensions, etc?
