
#include <pdal/pdal_test_main.hpp>

#include <pdal/PDALUtils.hpp>

#include "Support.hpp"

namespace pdal
{

TEST(ArbiterUtilsTest, tempFilename)
{
    // arbiter::getTempPath (called in Utils::tempFilename()) gets tmp directory 
    // from the environment. Can't think of a good way to test this consistently

    // should make a mock overload of arbiter::getTempPath() that returns Support::temppath()
    // not sure if that's possible, and it would break if/when arbiter is removed
}

TEST(ArbiterUtilsTest, fileSize)
{
    std::string s3("s3://pdal/test.txt");
    std::string local(Support::datapath("text/file1.txt"));
    // find http file to test too?
    // size = Utils::fileSize("https://s3.amazonaws.com/pdal/test.txt");

    uintmax_t size = Utils::fileSize(s3);
    std::cout << size << '\n';

    size = Utils::fileSize(local);
    std::cout << size << '\n';
}

TEST(ArbiterUtilsTest, createFile)
{
    // also requires tempFilename()
}

TEST(ArbiterUtilsTest, fetchRemote)
{
    // tempFilename()
}

TEST(ArbiterUtilsTest, openFile)
{
    // tempFilename()
}

TEST(ArbiterUtilsTest, fileExists)
{
    std::string s3("s3://pdal/test.txt");
    std::string local(Support::datapath("text/file1.txt"));
    // add http file or other drivers

    EXPECT_TRUE(Utils::fileExists(s3));
    EXPECT_TRUE(Utils::fileExists(local));
}

TEST(ArbiterUtilsTest, globTest)
{

}

}//namespace pdal
