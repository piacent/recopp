#include "cygnolib.h"
#include <iostream>
#include "s3.h"
#include <zlib.h>
#include <stdexcept>

// Appunti:
// classe evento
// immagine solo pixel del cluster + wfs connesse

int main() {
    
    //open midas file
    std::string filename="/home/piacenst/cygno_workspace/stefano/recopp/debug/run35138.mid.gz";
    
    //reading PMT readout infos from midas file
    std::vector<float> channels_offsets;
    bool correction;
    cygnolib::InitializePMTReadout(filename, &correction, &channels_offsets);
    
    //reading data from midas file
    TMReaderInterface* reader = cygnolib::OpenMidasFile(filename);
    bool reading = true;
    
    int counter =0;
    
    while (reading) {
        
        
        TMidasEvent event = TMidasEvent();
        reading = TMReadEvent(reader, &event);
        
        bool cam_found = cygnolib::FindBankByName(event, "CAM0");
        bool dgh_found = cygnolib::FindBankByName(event, "DGH0");
        bool dig_found = cygnolib::FindBankByName(event, "DIG0");
        
        if(dgh_found) {
            std::cout<<"Reading evt "<<counter<<std::endl;
            //std::cout<<"DGH0 bank found"<<std::endl;
            cygnolib::DGHeader dgh = cygnolib::daq_dgh2head(event);
            //dgh.Print();
            
        
            if(dig_found) {
                //std::cout<<"DIG0 bank found"<<std::endl;
                cygnolib::PMTData pmts = daq_dig2PMTData(event, &dgh);
                
                std::vector<std::vector<std::vector<uint16_t>>> *fastwfs = pmts.GetWaveforms(1742);
                std::vector<std::vector<std::vector<uint16_t>>> *slowwfs = pmts.GetWaveforms(1720);
                
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
        if(cam_found) {
            cygnolib::Picture pic=cygnolib::daq_cam2pic(event, "fusion");
            //pic.Print(4,4); // print upper left 4x4 angle
            //pic.SavePng("/data11/cygno/piacenst/stefano/cygnocpp/debug/test.png");
            
            counter ++;
        }
        
        
    }
    reader->Close();
    delete reader;
    
    return 0;
}
