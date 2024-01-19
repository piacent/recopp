#ifndef __CYGNO_LIB_H__
#define __CYGNO_LIB_H__

#include "midasio.h"
#include "mvodb.h"
#include "TMidasEvent.h"
#include <zlib.h>
#include <stdio.h>
#include <chrono>
#include <string>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <list>

namespace cygnolib {
    
    class Picture {
    public:
        Picture(unsigned int height = 2304, unsigned int width = 2304);
        ~Picture();
        
        unsigned int GetNRows();
        unsigned int GetNColumns();
        std::vector<std::vector<uint16_t>> GetFrame();
        void SetFrame(std::vector<std::vector<uint16_t>> inputframe);
        void Print(int a, int b);
        void SavePng(std::string filename, int vmin = 99, int vmax = 130);
        
    private:
        unsigned int nrows;
        unsigned int ncolumns;
        std::vector<std::vector<uint16_t>> frame;
    };
    // Appunti:
    // Almeno C++17/C++20 , con i vector
    // #include <algorithm> <----- algoritmi per i vector automaticamente parallelizzati
    // smart pointers (funzioni ritornano smart pointer invece che oggetti per ridurre memory usage)
    
    class DGHeader {
    public:
        
        DGHeader(std::vector<uint32_t> rawheader);
        ~DGHeader();
        
        void Print();
        
        int nboards;
        std::vector<int> board_model;
        std::vector<int> nwaveforms;
        std::vector<int> nchannels;
        std::vector<int> nsamples;
        std::vector<int> vertical_resolution;
        std::vector<int> sampling_rate;
        std::vector<std::vector<int>> offsets;
        std::vector<std::vector<int>> TTT;
        std::vector<std::vector<int>> SIC;
        
    };
    
    class PMTData {
    public:
        
        PMTData(DGHeader *DGH, std::vector<uint16_t> rawwaveforms);
        ~PMTData();
        
        std::list<std::vector<std::vector<std::vector<uint16_t>>>> data;
        
        std::vector<std::vector<std::vector<uint16_t>>> *GetWaveforms(int board_model);
        
        //void ApplyDRS4Corrections(std::vector<float> *channels_offsets);
        
    private:
        DGHeader *fDGH;
    };
    
    
    
    TMReaderInterface* OpenMidasFile(std::string filename);
    bool FindBankByName(TMidasEvent &event, std::string bname, bool verbose = false);
    
    bool FindODBDumpBOR(TMidasEvent &event, bool verbose = false);
    MVOdb* GetODBDumpBOR(TMidasEvent &event,  MVOdbError *odberror = NULL);
    
    void InitializePMTReadout(std::string filename, bool *DRS4correction, std::vector<float> *channels_offsets);
    // implementazione correzione to-do
    Picture  daq_cam2pic(TMidasEvent &event, std::string cam_model = "fusion");
    DGHeader daq_dgh2head(TMidasEvent &event);
    PMTData daq_dig2PMTData(TMidasEvent &event, DGHeader *DGH);
    
    
}

#endif
