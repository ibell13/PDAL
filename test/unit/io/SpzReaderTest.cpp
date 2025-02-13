
#include <pdal/pdal_test_main.hpp>

#include "Support.hpp"

#include <pdal/StageFactory.hpp>
#include <pdal/PipelineManager.hpp>

#include <io/SpzReader.hpp>

using namespace pdal;

TEST(SpzReaderTest, test1)
{
    StageFactory f;
    Options opts;
    opts.add("filename", Support::datapath("spz/fourth_st.spz"));
    PipelineManager mgr;
    
    Stage& reader = *f.createStage("readers.spz");
    reader.setOptions(opts);

    PointTable table;
    reader.prepare(table);
    PointViewSet set = reader.execute(table);
    PointViewPtr view = *set.begin();

    EXPECT_EQ(view->size(), 131199);
    ASSERT_TRUE(table.layout()->hasDim(Dimension::Id::Red));
    ASSERT_TRUE(table.layout()->hasDim(Dimension::Id::Alpha));
    //check custom dimensions too
}