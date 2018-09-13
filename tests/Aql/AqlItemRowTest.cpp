////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2018 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////


#include "catch.hpp"
#include "AqlItemBlockHelper.h"

#include "Aql/InputAqlItemRow.h"
#include "Aql/OutputAqlItemRow.h"

#include "Basics/VelocyPackHelper.h"

#include <velocypack/Builder.h>
#include <velocypack/Slice.h>
#include <velocypack/velocypack-aliases.h>

#include <iostream>
using namespace arangodb;
using namespace arangodb::aql;

namespace arangodb {
namespace tests {
namespace aql {

static void AssertEntry(InputAqlItemRow& in, RegisterId reg, VPackSlice value) {
  AqlValue v = in.getValue(reg);
  INFO("Expecting: " << value.toJson() << " got: " << v.slice().toJson());
  REQUIRE(basics::VelocyPackHelper::compare(value, v.slice(), true) == 0);
}

static void AssertResultMatrix(AqlItemBlock* in, VPackSlice result) {
  INFO("Expecting: " << result.toJson() << " Got: " << *in);
  REQUIRE(result.isArray());
  REQUIRE(in->size() == result.length());
  for (size_t i = 0; i < in->size(); ++i) {
    InputAqlItemRow validator{in, i, 0};
    VPackSlice row = result.at(i);
    REQUIRE(row.isArray());
    REQUIRE(in->getNrRegs() == row.length());
    for (RegisterId j = 0; j < in->getNrRegs(); ++j) {
      AqlValue v = validator.getValue(j);
      REQUIRE(basics::VelocyPackHelper::compare(row.at(j), v.slice(), true) == 0);
    }
  }
}

SCENARIO("AqlItemRows", "[AQL][EXECUTOR][ITEMROW]") {
  ResourceMonitor monitor;

  WHEN("only copying from source to target") {
    auto outputData = std::make_unique<AqlItemBlock>(&monitor, 3, 3);
    std::unordered_set<RegisterId> regsToKeep{0, 1, 2};


    OutputAqlItemRow testee(outputData.get(), 0, regsToKeep);

    THEN("the output rows need to be valid even if the source rows are gone") {
      {
        // Make sure this data is cleared before the assertions
        auto inputData = buildBlock<3>(&monitor, {
          {{ {1}, {2}, {3} }},
          {{ {4}, {5}, {6} }},
          {{ {"\"a\""}, {"\"b\""}, {"\"c\""} }}
        });

        InputAqlItemRow source{inputData.get(), 0, 0};

        testee.copyRow(source);
        REQUIRE(testee.produced());

        source = {inputData.get(), 1, 0};
        testee.advanceRow();
        testee.copyRow(source);
        REQUIRE(testee.produced());

        source = {inputData.get(), 2, 0};
        testee.advanceRow();
        testee.copyRow(source);
        REQUIRE(testee.produced());

        INFO("The input was " << (*(inputData.get())));
        INFO("The output is " << (*(outputData.get())));
        // TODO hard nullify source block
      }
      auto expected = VPackParser::fromJson("[[1,2,3],[4,5,6],[\"a\",\"b\",\"c\"]]");
      AssertResultMatrix(outputData.get(), expected->slice());
    }
  }

  WHEN("only copying from source to target but multiplying rows") {
    auto outputData = std::make_unique<AqlItemBlock>(&monitor, 9, 3);
    std::unordered_set<RegisterId> regsToKeep{0, 1, 2};

    OutputAqlItemRow testee(outputData.get(), 0, regsToKeep);

    THEN("the output rows need to be valid even if the source rows are gone") {
      {
        // Make sure this data is cleared before the assertions
        auto inputData = buildBlock<3>(&monitor, {
          {{ {1}, {2}, {3} }},
          {{ {4}, {5}, {6} }},
          {{ {"\"a\""}, {"\"b\""}, {"\"c\""} }}
        });


        for (size_t i = 0; i < 3; ++i) {
          // Iterate over source rows
          InputAqlItemRow source{inputData.get(), i, 0};
          for (size_t j = 0; j < 3; ++j) {
            testee.copyRow(source);
            REQUIRE(testee.produced());
            if (i < 2 || j < 2) {
              // Not at the last one, we are at the end
              testee.advanceRow();
            }
          }
        }
        // TODO hard nullify source block
      }
      auto expected = VPackParser::fromJson("["
        "[1,2,3],"
        "[1,2,3],"
        "[1,2,3],"
        "[4,5,6],"
        "[4,5,6],"
        "[4,5,6],"
        "[\"a\",\"b\",\"c\"],"
        "[\"a\",\"b\",\"c\"],"
        "[\"a\",\"b\",\"c\"]"
      "]");
      AssertResultMatrix(outputData.get(), expected->slice());
    }
  }
}

}  // namespace aql
}  // namespace tests
}  // namespace arangodb
