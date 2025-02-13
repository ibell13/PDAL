
#include <pdal/pdal_test_main.hpp>

#include "Support.hpp"

#include <pdal/StageFactory.hpp>
#include <pdal/PipelineManager.hpp>

#include <io/SpzReader.hpp>
#include <io/SpzWriter.hpp>

using namespace pdal;

TEST(SpzWriterTest, test1)
{
    StageFactory f;
    Options opts;
    opts.add("filename", Support::datapath("spz/fourth_st.spz"));
    PipelineManager mgr;
    
    Stage& reader = mgr.addReader("readers.spz");
    reader.setOptions(opts);

    Stage& writer = mgr.addWriter("writers.spz");
    opts.replace("filename", Support::datapath("test.spz"));
    writer.setInput(reader);
    writer.setOptions(opts);

    //std::cout << "point count : " << mgr.execute();
}

// add test for invalid dimensions, re-ordered dimensions, etc
