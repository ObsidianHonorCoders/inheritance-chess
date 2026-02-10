/// @file         test_main.cpp
/// @brief        Main test runner for Google Test
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @author       Calileus
/// @details      Entry point for running all unit tests using Google Test framework.
///               Initializes the testing environment and runs all registered tests.
/// @date         2026-02-09
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#include <gtest/gtest.h>
#include <iostream>

/// @brief   Main function for test runner
/// @param   argc Number of command line arguments
/// @param   argv Array of command line arguments
/// @return  Exit status code (0 for success, non-zero for failure)
/// @details Initializes Google Test framework, runs all tests, and returns results.
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  
  std::cout << "Running tests executed from main() defined in test_main.cpp...\n" << std::endl;
  int result = RUN_ALL_TESTS();
  if (result == 0)
  {
    std::cout << "\nAll tests passed successfully!" << std::endl;
  }
  else
  {
    std::cout << "\nSome tests failed. Please review the output above." << std::endl;
  }
  return result;
}
