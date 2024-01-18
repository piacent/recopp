#ifndef __S3_H__
#define __S3_H__

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace s3 {
    std::string mid_file(int run, std::string tag="LNGS", bool cloud = false, bool verbose=false);
}


#endif
