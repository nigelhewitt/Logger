#include "pch.h"
#include "CppUnitTest.h"

// Don't forget to do the 'Add reference' thing to ADIFlog
#include "../LogView/reader.cpp"
#include "../LogView/dxcc.cpp"
#include "../LogView/LogView.cpp"
#include "../LogView/config.cpp"
#include "../LogView/lotw.cpp"
#include "../LogView/ListViewChild.cpp"
#include "../LogView/eqsl.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
	TEST_CLASS(UnitTest1)
	{
	public:
		// length checks
		TEST_METHOD(TestMethod1)
		{
			size_t n = ITEM::length("ABC");
			size_t m = ITEM::length("DEF", "1234567890");
			Assert::IsTrue(n==6);
			Assert::IsTrue(m==19);
		}
		// write items
		TEST_METHOD(TestMethod2)
		{
			char temp[200]="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
			char *p = temp;
			Assert::IsTrue(ITEM::write(p, "EOR"));
			Assert::IsTrue(p-temp==6);
			Assert::IsTrue(strncmp(temp, "<EOR>\n", 6)==0);
			p = temp;
			Assert::IsTrue(ITEM::write(p, "TEST", "Value"));
			Assert::IsTrue(p-temp==14);
			Assert::IsTrue(strncmp(temp, "<TEST:5>Value ", 14)==0);
			p = temp;
			Assert::IsTrue(ITEM::write(p, "LF", "With linefeed", true));
			Assert::IsTrue(p-temp==21);
			Assert::IsTrue(strncmp(temp, "<LF:13>With linefeed\n", 21)==0);
		}
		// read items
		TEST_METHOD(TestMethod3)
		{
			char temp1[200]="  <EOR> lots of silly", *p1=temp1;
			auto res1 = ITEM::read(p1);
			Assert::IsTrue(p1-temp1==7);
			Assert::IsTrue(strcmp(res1->name, "EOR")==0 && res1->value==nullptr);
			delete res1;
			char temp2[200]="  <DOG:17>An Alsatian puppy lots of silly", *p2=temp2;
			auto res2 = ITEM::read(p2);
			Assert::IsTrue(p2-temp2==27);
			Assert::IsTrue(strcmp(res2->name, "DOG")==0 && strcmp(res2->value, "An Alsatian puppy")==0);
			delete res2;
		}
		// read items
		TEST_METHOD(TestMethod4)
		{
			ADIF log;
			Assert::IsTrue(log.read("..\\..\\wsjtx_log.adi"));
		}
		TEST_METHOD(TestMethod5)
		{
			char t1[] = "asb#*  ";
			strip_trailing(t1, " *#");
			Assert::IsTrue(strcmp(t1, "asb")==0);
			char t2[] = "ABC(34)(56),FG(5)";
			strip_brackets(t2);
			Assert::IsTrue(strcmp(t2, "ABC,FG")==0);
		}

		bool closerthan(double a, double b, double c){ return abs(a-b) < c; }
		TEST_METHOD(TestMethod6)
		{
			double latitude, longitude;
			square2spherical("IO90wt", latitude, longitude);
			Assert::IsTrue(closerthan(latitude, 50.818793, 1/24.0));
			Assert::IsTrue(closerthan(longitude, -0.108248, 1/12.0));

			int n = mileage("QF55", "IO90WT");
			Assert::IsTrue(n == 10617);
		}
	};
}
