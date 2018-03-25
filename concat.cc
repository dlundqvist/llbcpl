// Copyright 2017 Daniel Lundqvist. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <cstdio>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace std::string_literals;

int main(int argc, char const *argv[])
{
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <destination> <source files...>\n";
    return 1;
  }

  std::string oname(argv[1]);
  auto onametmp = oname + ".tmp";
  std::ofstream ofile(onametmp, std::ios::binary | std::ios::trunc);
  if (!ofile) {
    std::cerr << "ERROR: Unable to open destination file '"s << oname << "'\n";
    return 1;
  }

  for (auto i = 2; i < argc; ++i) {
    std::ifstream ifile(argv[i], std::ios::binary);
    if (!ifile) {
      std::cerr << "ERROR: Unable to open source file '" << argv[i] << "'\n";
      return 1;
    }
    std::copy(std::istream_iterator<unsigned char>(ifile >> std::noskipws),
              std::istream_iterator<unsigned char>(),
              std::ostream_iterator<unsigned char>(ofile));
  }
  std::rename(onametmp.c_str(), oname.c_str());
}
