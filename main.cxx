#include "cygnolib.h"
#include <iostream>
#include "s3.h"
#include <zlib.h>
#include <stdexcept>
#include <chrono>

// Appunti:
// classe evento
// immagine solo pixel del cluster + wfs connesse

int main() {
    
    bool debug = true;
    
    //open midas file
    std::string filename="/home/piacenst/cygno_workspace/stefano/recopp/debug/run35138.mid.gz";
    
    //reading PMT readout infos from midas file
    std::vector<float> channels_offsets;
    std::vector<std::vector<int>>    table_cell;
    std::vector<std::vector<int>> table_nsample;
    bool correction;
    cygnolib::InitializePMTReadout(filename, &correction, &channels_offsets, "LNGS", table_cell, table_nsample);
    
    
    //reading data from midas file
    TMReaderInterface* reader = cygnolib::OpenMidasFile(filename);
    bool reading = true;
    
    int counter =0;
    
    auto start00 = std::chrono::high_resolution_clock::now();
    
    while (reading) {
        
        auto start0 = std::chrono::high_resolution_clock::now();
        if(debug) std::cout<<"Reading evt "<<counter<<std::endl;
        TMidasEvent event = TMidasEvent();
        reading = TMReadEvent(reader, &event);
        if (!reading) {
            if(debug) std::cout<<"EOF reached."<<std::endl;
            break;
        }
        
        bool cam_found = cygnolib::FindBankByName(event, "CAM0");
        bool dgh_found = cygnolib::FindBankByName(event, "DGH0");
        bool dig_found = cygnolib::FindBankByName(event, "DIG0");
        
        auto stop0 = std::chrono::high_resolution_clock::now();
        auto duration0 = std::chrono::duration_cast<std::chrono::milliseconds>(stop0 - start0);
        if(debug) std::cout<<">> TIME TO READ MIDASEVENT "<< duration0.count()<<" ms"<<std::endl;
        
        if(dgh_found) {
            auto start = std::chrono::high_resolution_clock::now();
            //std::cout<<"Reading evt "<<counter<<std::endl;
            //std::cout<<"DGH0 bank found"<<std::endl;
            cygnolib::DGHeader dgh = cygnolib::daq_dgh2head(event);
            //dgh.Print();
            
        
            if(dig_found) {
                //std::cout<<"DIG0 bank found"<<std::endl;
                cygnolib::PMTData pmts = daq_dig2PMTData(event, &dgh);
                pmts.ApplyDRS4Corrections(&channels_offsets, &table_cell, &table_nsample);
                
                std::vector<std::vector<std::vector<uint16_t>>> *fastwfs = pmts.GetWaveforms(1742);
                std::vector<std::vector<std::vector<uint16_t>>> *slowwfs = pmts.GetWaveforms(1720);
                
                
                bool print = false;
                if(print) {
                    std::cout<<"====== fast ====="<<std::endl;
                    int ev = 0;
                    int ch = 1;
                    for(int i =0; i<10; i++) {
                        std::cout<<(*fastwfs)[ev][ch][i]<<", ";
                    }
                    std::cout<<std::endl;
                    std::cout<<"====== slow ====="<<std::endl;
                    for(int i =0; i<10; i++) {
                        std::cout<<(*slowwfs)[ev][ch][i]<<", ";
                    }
                    std::cout<<std::endl;
                }
                
            }
            
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            if(debug) std::cout<<">> TIME TO INIT DGH0 AND DIG0 "<< duration.count()<<" ms"<<std::endl;
        }
        
        if(cam_found) {
            auto start = std::chrono::high_resolution_clock::now();
            cygnolib::Picture pic=cygnolib::daq_cam2pic(event, "fusion");
            //pic.Print(4,4); // print upper left 4x4 angle
            //pic.SavePng("/data11/cygno/piacenst/stefano/cygnocpp/debug/test.png");
            
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            if(debug) std::cout<<">> TIME TO INIT CAM0 "<< duration.count()<<" ms"<<std::endl;
        }
        
        counter ++;
        
    }
    
    reader->Close();
    delete reader;
    auto stop00 = std::chrono::high_resolution_clock::now();
    auto duration00 = std::chrono::duration_cast<std::chrono::milliseconds>(stop00 - start00);
    if(debug) std::cout<<">> TIME TO READ ALL MIDAS FILE "<< duration00.count()<<" ms"<<std::endl;
    
    return 0;
}
