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


/**
 * @brief Functions to interface with the cloud services
 *
 *
 */
namespace s3 {
    
    /**
     * @brief This function identifies the string of the MIDAS datafile based on the
     * run number and the location of the file
     *
     * @param[in] run run number
     * @param[in] tag tag of the bucket the datafile belongs to (LNGS, LNF, ...). Default is "LNGS".
     * @param[in] cloud flag to look for the data on the cloud. Default is true.
     * @param[in] verbose flag for verbose mode. Default is false.
     *
     * @return the string of the MIDAS datafile
     *
     */
    std::string mid_file(int run, std::string tag="LNGS", bool cloud = true, bool verbose=false);
    
    
    /**
     * @brief This function identifies the location of the MIDAS datafile based on its name
     * as given by the s3::mid_file function. If the file is not found, it is then
     * downloaded from the cloud.
     *
     * @param[in] fname the string of the MIDAS datafile
     * @param[in] path path where the MIDAS file will be stored on the local disk. Default
     * is "./tmp/".
     * @param[in] cloud flag to look for the data on the cloud. Default is true.
     * @param[in] tag tag of the bucket the datafile belongs to (LNGS, LNF, ...). Default is "LNGS".
     * @param[in] verbose flag for verbose mode. Default is false.
     *
     * @return the filepath to the MIDAS datafile
     *
     */
    std::string cache_file(std::string fname,
                           std::string path  = "./tmp/", 
                           bool        cloud = true,
                           std::string tag   = "LNGS",
                           bool      verbose = false);
}


#endif
