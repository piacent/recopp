/*
 * Copyright (C) 2024 CYGNO Collaboration
 *
 *
 * Author: Stefano Piacentini
 * Created in 2024
 *
 */

#ifndef __S3_H__
#define __S3_H__

#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>



namespace s3 {
    std::string mid_file(int run, std::string tag="LNGS", bool cloud = false, bool verbose=false);
    
    std::string cache_file(std::string fname,
                           std::string path  = "./tmp/", 
                           bool        cloud = true,
                           std::string tag   = "LNGS",
                           bool      verbose = false);
}


#endif
