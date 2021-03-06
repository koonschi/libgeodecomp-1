#include <libgeodecomp/misc/stringops.h>

#include <cxxtest/TestSuite.h>

using namespace LibGeoDecomp;

namespace LibGeoDecomp {

class StringOpsTest : public CxxTest::TestSuite
{
public:

    void testItoa()
    {
        TS_ASSERT_EQUALS("0",   StringOps::itoa(0));
        TS_ASSERT_EQUALS("-1",  StringOps::itoa(-1));
        TS_ASSERT_EQUALS("123", StringOps::itoa(123));
    }

    void testAtoi()
    {
        TS_ASSERT_EQUALS(0,   StringOps::atoi("0"  ));
        TS_ASSERT_EQUALS(-1,  StringOps::atoi("-1" ));
        TS_ASSERT_EQUALS(123, StringOps::atoi("123"));
    }

    void testAtof()
    {
        TS_ASSERT_EQUALS(0,   StringOps::atof("0"  ));
        TS_ASSERT_EQUALS(-1,  StringOps::atof("-1" ));
        TS_ASSERT_EQUALS(123, StringOps::atof("123"));

        TS_ASSERT_EQUALS(0.25, StringOps::atof("0.25"));
        TS_ASSERT_EQUALS(-0.5, StringOps::atof("-0.5"));
    }

    void testTokenize()
    {
        std::string message = "abc_123_andi ist so toll";

        StringVec expected1;
        StringVec expected2;
        StringVec expected3;

        expected1 << "abc"
                  << "123"
                  << "andi ist so toll";

        expected2 << "abc"
                  << "123"
                  << "andi"
                  << "ist"
                  << "so"
                  << "toll";

        TS_ASSERT_EQUALS(expected1, StringOps::tokenize(message, "_"));
        TS_ASSERT_EQUALS(expected2, StringOps::tokenize(message, "_ "));
        StringVec tokens = StringOps::tokenize("\n", " \n");
        TS_ASSERT_EQUALS(expected3, StringOps::tokenize("\n", " \n"));
    }

    void testTokenize2()
    {
        std::string message = " x y ";
        StringVec tokens = StringOps::tokenize(message, " ");

        StringVec expectedTokens;
        expectedTokens << "x"
                       << "y";

        TS_ASSERT_EQUALS(tokens, expectedTokens);
    }

    void testJoin()
    {
        StringVec tokens;
        tokens << "a"
               << "bb"
               << "ccc";
        TS_ASSERT_EQUALS("a--bb--ccc", StringOps::join(tokens, "--"));
        TS_ASSERT_EQUALS("a bb ccc", StringOps::join(tokens, " "));
        TS_ASSERT_EQUALS("abbccc", StringOps::join(tokens, ""));
    }
};

}
