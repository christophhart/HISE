/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   ===========================================================================
*/

namespace hise {
using namespace juce;

class SemanticVersionCheckerTests : public UnitTest
{
public:

	SemanticVersionCheckerTests() : UnitTest("SemanticVersionChecker Tests", "Misc Tools") {}

	void runTest() override
	{
		testThreePartParsing();
		testFourPartParsing();
		testInvalidVersions();
		testIsUpdateThreePart();
		testIsUpdateFourPart();
		testIsUpdateMixedParts();
		testExactMatch();
		testGranularityChecks();
		testToString();
		testVPrefixStripping();
		testArrayConstructors();
	}

private:

	void testThreePartParsing()
	{
		beginTest("Three-part version parsing");

		SemanticVersionChecker c1("1.0.0", "1.0.0");
		expect(c1.oldVersionNumberIsValid(), "1.0.0 should be valid");
		expect(c1.newVersionNumberIsValid(), "1.0.0 should be valid");

		SemanticVersionChecker c2("2.3.1", "0.1.0");
		expect(c2.oldVersionNumberIsValid(), "2.3.1 should be valid");
		expect(c2.newVersionNumberIsValid(), "0.1.0 should be valid");
	}

	void testFourPartParsing()
	{
		beginTest("Four-part version parsing");

		SemanticVersionChecker c1("1.4.1.125", "1.4.1.125");
		expect(c1.oldVersionNumberIsValid(), "1.4.1.125 should be valid");
		expect(c1.newVersionNumberIsValid(), "1.4.1.125 should be valid");

		SemanticVersionChecker c2("1.0.0.0", "2.0.0.1");
		expect(c2.oldVersionNumberIsValid(), "1.0.0.0 should be valid");
		expect(c2.newVersionNumberIsValid(), "2.0.0.1 should be valid");
	}

	void testInvalidVersions()
	{
		beginTest("Invalid version strings");

		SemanticVersionChecker c1("1.0", "1.0");
		expect(!c1.oldVersionNumberIsValid(), "1.0 should be invalid");

		SemanticVersionChecker c2("abc", "1.0.0");
		expect(!c2.oldVersionNumberIsValid(), "abc should be invalid");

		SemanticVersionChecker c3("1.0.0.1.2", "1.0.0");
		expect(!c3.oldVersionNumberIsValid(), "1.0.0.1.2 (5 parts) should be invalid");

		SemanticVersionChecker c4("1", "1.0.0");
		expect(!c4.oldVersionNumberIsValid(), "1 should be invalid");

		SemanticVersionChecker c5("", "1.0.0");
		expect(!c5.oldVersionNumberIsValid(), "empty string should be invalid");
	}

	void testIsUpdateThreePart()
	{
		beginTest("isUpdate with three-part versions");

		/** Setup: Two three-part version strings
		 *  Scenario: Major version increase
		 *  Expected: isUpdate returns true
		 */
		expect(SemanticVersionChecker("1.0.0", "2.0.0").isUpdate(), "2.0.0 > 1.0.0");
		expect(!SemanticVersionChecker("2.0.0", "1.0.0").isUpdate(), "1.0.0 < 2.0.0");

		expect(SemanticVersionChecker("1.0.0", "1.1.0").isUpdate(), "1.1.0 > 1.0.0");
		expect(!SemanticVersionChecker("1.1.0", "1.0.0").isUpdate(), "1.0.0 < 1.1.0");

		expect(SemanticVersionChecker("1.0.0", "1.0.1").isUpdate(), "1.0.1 > 1.0.0");
		expect(!SemanticVersionChecker("1.0.1", "1.0.0").isUpdate(), "1.0.0 < 1.0.1");

		expect(!SemanticVersionChecker("1.0.0", "1.0.0").isUpdate(), "1.0.0 == 1.0.0");

		// Higher major always wins regardless of minor/patch
		expect(!SemanticVersionChecker("2.0.0", "1.9.9").isUpdate(), "major takes precedence");
		expect(!SemanticVersionChecker("1.2.0", "1.1.9").isUpdate(), "minor takes precedence over patch");
	}

	void testIsUpdateFourPart()
	{
		beginTest("isUpdate with four-part versions");

		expect(SemanticVersionChecker("1.0.0.1", "1.0.0.2").isUpdate(), "build 2 > build 1");
		expect(!SemanticVersionChecker("1.0.0.2", "1.0.0.1").isUpdate(), "build 1 < build 2");
		expect(!SemanticVersionChecker("1.0.0.5", "1.0.0.5").isUpdate(), "same build");

		expect(SemanticVersionChecker("1.0.0.100", "1.0.0.200").isUpdate(), "build 200 > build 100");

		// Higher components still take precedence
		expect(SemanticVersionChecker("1.0.0.999", "1.0.1.0").isUpdate(), "patch wins over build");
		expect(SemanticVersionChecker("1.0.9.999", "1.1.0.0").isUpdate(), "minor wins over patch+build");
		expect(SemanticVersionChecker("1.9.9.999", "2.0.0.0").isUpdate(), "major wins over all");
	}

	void testIsUpdateMixedParts()
	{
		beginTest("isUpdate with mixed three/four-part versions");

		/** Setup: Compare a 3-part version against a 4-part version
		 *  Scenario: 3-part defaults buildNumber to 0
		 *  Expected: 1.0.0.1 is an update over 1.0.0 (build 0)
		 */
		expect(SemanticVersionChecker("1.0.0", "1.0.0.1").isUpdate(), "1.0.0.1 > 1.0.0 (build defaults to 0)");
		expect(!SemanticVersionChecker("1.0.0.1", "1.0.0").isUpdate(), "1.0.0 < 1.0.0.1");
		expect(!SemanticVersionChecker("1.0.0", "1.0.0.0").isUpdate(), "1.0.0.0 == 1.0.0 (both build 0)");
	}

	void testExactMatch()
	{
		beginTest("isExactMatch");

		expect(SemanticVersionChecker("1.0.0", "1.0.0").isExactMatch(), "1.0.0 == 1.0.0");
		expect(SemanticVersionChecker("1.0.0.5", "1.0.0.5").isExactMatch(), "1.0.0.5 == 1.0.0.5");
		expect(!SemanticVersionChecker("1.0.0", "1.0.0.1").isExactMatch(), "1.0.0 != 1.0.0.1");
		expect(!SemanticVersionChecker("1.0.0", "1.0.1").isExactMatch(), "1.0.0 != 1.0.1");
		expect(SemanticVersionChecker("1.0.0", "1.0.0.0").isExactMatch(), "1.0.0 == 1.0.0.0 (both build 0)");
	}

	void testGranularityChecks()
	{
		beginTest("Granularity check methods");

		SemanticVersionChecker major("1.0.0", "2.0.0");
		expect(major.isMajorVersionUpdate(), "major should be flagged");
		expect(!major.isMinorVersionUpdate(), "minor should not be flagged");
		expect(!major.isPatchVersionUpdate(), "patch should not be flagged");
		expect(!major.isBuildNumberUpdate(), "build should not be flagged");

		SemanticVersionChecker minor("1.0.0", "1.1.0");
		expect(!minor.isMajorVersionUpdate());
		expect(minor.isMinorVersionUpdate());
		expect(!minor.isPatchVersionUpdate());
		expect(!minor.isBuildNumberUpdate());

		SemanticVersionChecker patch("1.0.0", "1.0.1");
		expect(!patch.isMajorVersionUpdate());
		expect(!patch.isMinorVersionUpdate());
		expect(patch.isPatchVersionUpdate());
		expect(!patch.isBuildNumberUpdate());

		SemanticVersionChecker build("1.0.0.1", "1.0.0.2");
		expect(!build.isMajorVersionUpdate());
		expect(!build.isMinorVersionUpdate());
		expect(!build.isPatchVersionUpdate());
		expect(build.isBuildNumberUpdate());
	}

	void testToString()
	{
		beginTest("getErrorMessage / toString output");

		SemanticVersionChecker c1("1.0.0", "2.3.1");
		auto msg1 = c1.getErrorMessage("old", "new");
		expect(msg1.contains("1.0.0"), "should contain old version");
		expect(msg1.contains("2.3.1"), "should contain new version");

		SemanticVersionChecker c2("1.0.0.0", "1.4.1.125");
		auto msg2 = c2.getErrorMessage("old", "new");
		// buildNumber 0 is omitted from toString
		expect(msg2.contains("1.0.0"), "should contain old version without build 0");
		expect(msg2.contains("1.4.1.125"), "should contain new version with build number");
	}

	void testVPrefixStripping()
	{
		beginTest("v-prefix stripping");

		SemanticVersionChecker c1("v1.0.0", "v2.0.0");
		expect(c1.oldVersionNumberIsValid(), "v1.0.0 should be valid");
		expect(c1.newVersionNumberIsValid(), "v2.0.0 should be valid");
		expect(c1.isUpdate(), "v2.0.0 > v1.0.0");

		SemanticVersionChecker c2("v1.0.0.5", "v1.0.0.10");
		expect(c2.oldVersionNumberIsValid(), "v1.0.0.5 should be valid");
		expect(c2.isUpdate(), "v1.0.0.10 > v1.0.0.5");
	}

	void testArrayConstructors()
	{
		beginTest("Array constructors");

		// std::array<int, 3> constructor
		std::array<int, 3> old3 = { 1, 0, 0 };
		std::array<int, 3> new3 = { 1, 1, 0 };
		SemanticVersionChecker c3(old3, new3);
		expect(c3.oldVersionNumberIsValid());
		expect(c3.newVersionNumberIsValid());
		expect(c3.isUpdate(), "1.1.0 > 1.0.0 via array<3>");

		// std::array<int, 4> constructor
		std::array<int, 4> old4 = { 1, 0, 0, 5 };
		std::array<int, 4> new4 = { 1, 0, 0, 10 };
		SemanticVersionChecker c4(old4, new4);
		expect(c4.oldVersionNumberIsValid());
		expect(c4.newVersionNumberIsValid());
		expect(c4.isUpdate(), "1.0.0.10 > 1.0.0.5 via array<4>");
		expect(c4.isBuildNumberUpdate(), "build 10 > build 5 via array<4>");
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SemanticVersionCheckerTests);
};

static SemanticVersionCheckerTests semanticVersionCheckerTests;

} // namespace hise
