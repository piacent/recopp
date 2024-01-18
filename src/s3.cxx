#include "s3.h"
#include <sstream>
#include <iomanip>

namespace s3 {
    std::string BAKET_POSIX_PATH = "/jupyter-workspace/cloud-storage/";
    std::string BAKET_REST_PATH = "https://s3.cloud.infn.it/v1/AUTH_2ebf769785574195bde2ff418deac08a/";

    std::string mid_file(int run, std::string tag, bool cloud, bool verbose) {
        std::stringstream ss;
        ss << std::setw(5) << std::setfill('0') << run;
        std::string srun = ss.str();
        
        std::string BASE_URL;
        std::string f;
        
        if(cloud) {
            BASE_URL = BAKET_REST_PATH+"cygno-data/";
            f = BASE_URL+tag+"/run"+srun+".mid.gz";
        } else {
            f = "/run"+srun+".mid.gz";
        }
        if (verbose) std::cout<<f<<std::endl;
        return f;
    }
}
