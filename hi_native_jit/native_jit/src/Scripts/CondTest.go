// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

package main

// Quick hack script to generate tests for conditionals.

import (
	"fmt"
	"strings"
)

// TODO: conditionals listed below aren't tested.
/*
        JO
        JNO
        JAE
	JNB
	JNC
        JS
        JNS
        JP
	JPE
        JNP
	JPO
*/

type CppType int

const (
	SIGNED CppType = iota
	UNSIGNED
	BOTH
)

var (
	template = `        TEST_CASE_F(ConditionalAutoGen, REPLACE_NAME)
        {
            auto setup = GetSetup();

            {
                Function<uint64_t, uint64_t, uint64_t> expression(setup->GetAllocator(), setup->GetCode());

                uint64_t trueValue = 1;
                uint64_t falseValue = 128;

                auto & a = expression.Compare<JccType::REPLACE_COND>(expression.GetP1(), expression.GetP2());
                auto & b = expression.Conditional(a, expression.Immediate(trueValue), expression.Immediate(falseValue));
                auto function = expression.Compile(b);

                for (REPLACE_TYPE p1 = REPLACE_STARTVAL; p1 <= 1; ++p1) {
                  for (REPLACE_TYPE p2 = REPLACE_STARTVAL; p2 <= 1; ++p2) {
                    auto expected = (REPLACE_EXPR) ? trueValue : falseValue;
                    auto observed = function(p1, p2);

                    TestEqual(expected, observed);
                  }
                }
            }
        }
`
	// http://unixwiz.net/techtips/x86-jumps.html
	condExpr = map[string]string{
		"JNE": "p1 != p2",
		"JNZ": "p1 != p2",
		"JE":  "p1 == p2",
		"JZ":  "p1 == p2",
		"JB":  "p1 < p2",
		// JC (same as JB) appears to be missing. Is that a bug?
		"JBE":  "p1 <= p2",
		"JNA":  "p1 <= p2",
		"JA":   "p1 > p2",
		"JNBE": "p1 > p2",
		"JL":   "p1 < p2",
		"JNGE": "p1 < p2",
		"JGE":  "p1 >= p2",
		"JNL":  "p1 >= p2",
		"JLE":  "p1 <= p2",
		"JNG":  "p1 <= p2",
		"JG":   "p1 > p2",
		"JNLE": "p1 > p2",
	}

	condType = map[string]CppType{
		"JNE":  BOTH,
		"JNZ":  BOTH,
		"JE":   BOTH,
		"JZ":   BOTH,
		"JB":   UNSIGNED,
		"JBE":  UNSIGNED,
		"JNA":  UNSIGNED,
		"JA":   UNSIGNED,
		"JNBE": UNSIGNED,
		"JL":   SIGNED,
		"JNGE": SIGNED,
		"JGE":  SIGNED,
		"JNL":  SIGNED,
		"JLE":  SIGNED,
		"JNG":  SIGNED,
		"JG":   SIGNED,
		"JNLE": SIGNED,
	}
)

func createTest(testName string, expr string, cond string, typeInTest string, startVal string) {
	thisTest := strings.Replace(template, "REPLACE_NAME", testName, 1)
	thisTest = strings.Replace(thisTest, "REPLACE_EXPR", expr, 1)
	thisTest = strings.Replace(thisTest, "REPLACE_COND", cond, 1)
	thisTest = strings.Replace(thisTest, "REPLACE_TYPE", typeInTest, -1)
	thisTest = strings.Replace(thisTest, "REPLACE_STARTVAL", startVal, -1)
	fmt.Println(thisTest)
}

func main() {
	for cond, expr := range condExpr {
		testName := "AutoGen" + cond

		if condType[cond] == UNSIGNED || condType[cond] == BOTH {
			createTest(testName+"Unsigned", expr, cond, "uint64_t", "0")
		}
		if condType[cond] == SIGNED || condType[cond] == BOTH {
			createTest(testName+"Signed", expr, cond, "int64_t", "-1")
		}
	}
}
