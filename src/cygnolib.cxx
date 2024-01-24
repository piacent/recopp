#include "cygnolib.h"
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
#include "date.h"
#include <cmath>
#include <stdexcept>
#include <opencv2/imgcodecs.hpp>
#include <list>
#include <cstdlib>


namespace cygnolib {
    
    Picture::Picture(unsigned int height, unsigned int width): nrows(height), ncolumns(width), frame(height, std::vector<uint16_t>(width,0)) {
    }
    Picture::~Picture(){
    }
    unsigned int Picture::GetNRows(){
        return nrows;
    }
    unsigned int Picture::GetNColumns(){
        return ncolumns;
    }
    std::vector<std::vector<uint16_t>> Picture::GetFrame(){
        return frame;
    }
    void Picture::SetFrame(std::vector<std::vector<uint16_t>> inputframe){
        unsigned int height = inputframe.size();
        unsigned int width  = inputframe[0].size();
        if(height!=nrows || width!=ncolumns) {
            throw std::invalid_argument("cygnolib::Picture::SetFrame: input frame has wrong dimensions.\n");
        }
        frame = inputframe;
    }
    void Picture::Print(int a, int b) {
        for(int i=0;i<a;i++) {
            for(int j=0;j<b;j++) {
                std::cout<<frame[i][j]<<",\t";
            }
            std::cout<<std::endl;
        }
    }
    void Picture::SavePng(std::string filename, int vmin, int vmax) {
        cv::Mat mat;
        mat.create(nrows, ncolumns, CV_16U); 
        for (unsigned int r = 0; r < nrows; ++r) {
            for (unsigned int c = 0; c < ncolumns; ++c) {
                uint16_t tmp = frame[r][c];
                if (tmp<vmin) tmp = vmin;
                else if (tmp>vmax) tmp = vmax;
                tmp = (tmp-vmin)*65535/(vmax-vmin);
                mat.at<uint16_t>(r, c) = tmp;
            }
        }
        cv::imwrite(filename, mat);
    }
     
    
    DGHeader::DGHeader(std::vector<uint32_t> rawheader) {
        if(rawheader.size()==0) {
            throw std::runtime_error("cygnolib::DGHeader::DGHeader: empty raw header.");
        }
        if(rawheader.size()<7) {
            throw std::runtime_error("cygnolib::DGHeader::DGHeader: corrupted.");
        }
        
        
        nboards=rawheader[0];
        
        int baseidx = 1;
        for(int i=0;i<nboards;i++) {
            
            board_model.push_back(rawheader[baseidx]);
            nsamples.push_back(rawheader[baseidx+1]);
            nchannels.push_back(rawheader[baseidx+2]);
            nwaveforms.push_back(rawheader[baseidx+3]);
            vertical_resolution.push_back(rawheader[baseidx+4]);
            sampling_rate.push_back(rawheader[baseidx+5]);
            
            
            if((int)rawheader.size()<baseidx+6+nchannels[i]+2*nwaveforms[i] && board_model[i] ==1742) {
                throw std::runtime_error("cygnolib::DGHeader::DGHeader: corrupted.");
                } else if((int)rawheader.size()<baseidx+6+nchannels[i]+nwaveforms[i] && board_model[i] ==1720) {
                throw std::runtime_error("cygnolib::DGHeader::DGHeader: corrupted.");
            }
            
            std::vector<int> tmp_offsets(nchannels[i], 0);
            for(int j=0;j<nchannels[i];j++){
                tmp_offsets[j] = rawheader[baseidx+6+j];
            }
            offsets.push_back(tmp_offsets);
            
            std::vector<int> tmp_TTT(nwaveforms[i], 0);
            for(int j=0;j<nwaveforms[i];j++){
                tmp_TTT[j] = rawheader[baseidx+6+nchannels[i]+j];
            }
            TTT.push_back(tmp_TTT);
            
            
            std::vector<int> tmp_SIC(nwaveforms[i], 0);
            for(int j=0;j<nwaveforms[i];j++){
                if(board_model[i]==1742){
                    tmp_SIC[j] = rawheader[baseidx+6+nchannels[i]+nwaveforms[i]+j];
                }
            }
            SIC.push_back(tmp_SIC);
            
            if(board_model[i] == 1742)      baseidx = baseidx + 6 + nchannels[i] + 2*nwaveforms[i];
            else if (board_model[i] ==1720) baseidx = baseidx + 6 + nchannels[i] +   nwaveforms[i];
            
        }
    }
    DGHeader::~DGHeader(){
    }
    void DGHeader::Print() {
        
        std::cout<<"=========================="<<std::endl;
        for(int i=0;i<nboards;i++){
            std::cout<<"Board # "<<i<<std::endl;
            std::cout<<"model:     "<<board_model[i]<<std::endl;
            std::cout<<"length:    "<<nsamples[i]<<std::endl;
            std::cout<<"channels:  "<<nchannels[i]<<std::endl;
            std::cout<<"nwfs:      "<<nwaveforms[i]<<std::endl;
            std::cout<<"vresol:    "<<vertical_resolution[i]<<std::endl;
            std::cout<<"samp.rate: "<<sampling_rate[i]<<std::endl;
            std::cout<<"--------------------------"<<std::endl;
            std::cout<<"channel offsets:"<<std::endl;
            for(unsigned int j=0;j<offsets[i].size();j++){
                std::cout<<offsets[i][j]<<", ";
            }
            std::cout<<std::endl;
            std::cout<<"--------------------------"<<std::endl;
            std::cout<<"event TTTs:"<<std::endl;
            for(unsigned int j=0;j<TTT[i].size();j++){
                std::cout<<TTT[i][j]<<", ";
            }
            std::cout<<std::endl;
            std::cout<<"--------------------------"<<std::endl;
            std::cout<<"event SICs:"<<std::endl;
            for(unsigned int j=0;j<SIC[i].size();j++){
                std::cout<<SIC[i][j]<<", ";
            }
            std::cout<<std::endl;
            std::cout<<"=========================="<<std::endl;
        }
    }
    
    
    PMTData::PMTData(DGHeader *DGH, std::vector<uint16_t> rawwaveforms): fDGH(DGH){
        int nboards = fDGH->nboards;
        
        int baseidx = 0;
        
        unsigned int totlength = 0;
        for(int i=0;i<nboards;i++) {
            int length_i = fDGH->nchannels[i]*fDGH->nsamples[i]*fDGH->nwaveforms[i];
            totlength += length_i;
        }
        if(totlength!=rawwaveforms.size()) {
            throw std::runtime_error("cygnolib::PMTData::PMTdata: corrupted.");
        }
        
        
        for(int i=0;i<nboards;i++) {
            int length_i = fDGH->nchannels[i]*fDGH->nsamples[i]*fDGH->nwaveforms[i];
            std::vector<uint16_t> rawwaveforms_i(length_i, 0);
            for(int j=0; j<length_i; j++) {
                rawwaveforms_i[j]=rawwaveforms[baseidx+j];
            }
            baseidx += length_i;
            
            std::vector<std::vector<std::vector<uint16_t>>> tmp(fDGH->nwaveforms[i],
                                                            std::vector<std::vector<uint16_t>>(fDGH->nchannels[i], 
                                                            std::vector<uint16_t>(fDGH->nsamples[i], 0)
                                                            )
                                                           );
            
            for(int evt=0; evt<fDGH->nwaveforms[i]; evt++) {
                for (int ch=0; ch<fDGH->nchannels[i]; ch++) {
                    for (int samp=0; samp<fDGH->nsamples[i]; samp++) {
                        int index = evt*fDGH->nchannels[i]*fDGH->nsamples[i] + ch*fDGH->nsamples[i] + samp;
                        tmp[evt][ch][samp] = rawwaveforms_i[index];
                    }
                }
            }
            
            data.push_back(tmp);
        }
    }
    PMTData::~PMTData(){
    }
    std::vector<std::vector<std::vector<uint16_t>>> *PMTData::GetWaveforms(int board_model) {
        int nboards = fDGH->nboards;
        bool board_found = false;
        int board_index = -1;
        for(int i=0;i<nboards;i++) {
            if(board_model==fDGH->board_model[i]) {
                board_found = true;
                board_index = i;
            }
        }
        if(!board_found) {
            throw std::runtime_error("cygnolib::PMTData::GetWaveforms: board model"+
                                     std::to_string(board_model)+
                                     " not found."
                                    );
        }
        
        auto data_front = data.begin();
        std::advance(data_front, board_index);
        
        return &(*data_front);
    }
    void PMTData::PeakCorrection(std::vector<std::vector<uint16_t>> &wfs) {
        unsigned int NS = 1024; //hardcoded!!! Valid only for V1742
        unsigned int Nch = 8;
        std::vector<double> avgs(Nch, 0.0);
        for(unsigned int ch=0; ch<Nch; ch++){
            avgs[ch] = std::accumulate(wfs[ch].begin(), wfs[ch].end(), 0.0) / NS; // averages of each channel 
        }
        for(unsigned int i =1; i<NS; i++) {
            int offset      = 0;
            int offset_plus = 0;
            
            for(unsigned int ch=0; ch<Nch; ch++){
                if(i ==1) {                          
                    if ((wfs[ch][2] - wfs[ch][1])>30) {
                        offset += 1;
                    } else {
                        if ((wfs[ch][3]-wfs[ch][1])>30 && (wfs[ch][3]-wfs[ch][2])>30) {
                            offset += 1;
                        }
                    }
                } else {
                    if (i == (NS-1) && (wfs[ch][NS-2] - wfs[ch][NS-1])>30) {
                        offset+=1;
                    } else {
                        if ((wfs[ch][i-1]-wfs[ch][i])>30) {
                            if ((wfs[ch][i+1] - wfs[ch][i])>30) {
                                offset += 1;
                            } else if ((i+2)<NS-2) {
                                if ((wfs[ch][i+2] - wfs[ch][i])>30 && (wfs[ch][i+1] - wfs[ch][i])<5) {
                                    offset += 1;
                                }
                            } else {
                                if (i == (NS-2) || (wfs[ch][i+2]-wfs[ch][i])>30) {
                                    offset += 1;
                                }
                            }
                        }
                    }
                }
                
                if ((i < (NS-6))                  &&
                    (avgs[ch] - wfs[ch][i])  <-30 &&
                    (avgs[ch] - wfs[ch][i+1])<-30 &&
                    (avgs[ch] - wfs[ch][i+2])<-30 &&
                    (avgs[ch] - wfs[ch][i+3])<-30 &&
                    (avgs[ch] - wfs[ch][i+4])<-30 &&
                    (avgs[ch] - wfs[ch][i+5])<-30 ) {
                    
                    offset_plus += 1;
                }
            }
            
            
            if (offset >= 7) { // 7 instead of 8 !!!!
                
                for(unsigned int ch=0; ch<Nch; ch++){
                    if (i ==1) {
                        if ((wfs[ch][2] - wfs[ch][1])>30){
                            wfs[ch][0] = wfs[ch][2];
                            wfs[ch][1] = wfs[ch][2];
                        } else {
                            wfs[ch][0] = wfs[ch][3];
                            wfs[ch][1] = wfs[ch][3];
                            wfs[ch][2] = wfs[ch][3];
                        }
                    } else {
                        if (i == (NS-1)) {
                            wfs[ch][NS-1] = wfs[ch][NS-2];
                        } else {
                            if ((wfs[ch][i+1]-wfs[ch][i])>30) {
                                if ((wfs[ch][i+1] - wfs[ch][i])>30) {
                                    wfs[ch][i]   =  int((wfs[ch][i+1]+ wfs[ch][i-1])/2);
                                } else if ((i+2)<NS-2) {
                                    if ((wfs[ch][i+2] - wfs[ch][i])>30 && (wfs[ch][i+1] - wfs[ch][i])<5){
                                        wfs[ch][i]   =  int((wfs[ch][i+2]+ wfs[ch][i-1])/2);
                                        wfs[ch][i+1] =  int((wfs[ch][i+2]+ wfs[ch][i-1])/2);
                                    }
                                }
                            } else {
                                if (i == (NS-2)){
                                    wfs[ch][NS-2] = wfs[ch][NS-3];
                                    wfs[ch][NS-2] = wfs[ch][NS-1-3];                 
                                } else {
                                    wfs[ch][i]   = int((wfs[ch][i+2]+wfs[ch][i-1])/2);
                                    wfs[ch][i+1] = int((wfs[ch][i+2]+wfs[ch][i-1])/2);
                                }
                            }
                        }
                    }
                }
            }
            
            
            if (offset_plus>=7) {  // 7 instead of 8 !!!!
                for(unsigned int ch=0; ch<Nch; ch++){
                    for(unsigned int m=0; m<6; m++){
                        wfs[ch][i+m] = avgs[ch];
                    }
                }
            }
        }
    }  
    void PMTData::ApplyDRS4Corrections(std::vector<float> *channel_offsets,
                                       std::vector<std::vector<int>> *table_cell,
                                       std::vector<std::vector<int>> *table_nsample) {
        int nboards = fDGH->nboards;
        bool board_found = false;
        int board_index = -1;
        for(int i=0;i<nboards;i++) {
            if(1742==fDGH->board_model[i]) {
                board_found = true;
                board_index = i;
            }
        }
        if(!board_found) {
            throw std::runtime_error("cygnolib::PMTData::ApplyDRS4Corrections: board model"+
                                     std::to_string(1742)+
                                     " not found."
                                    );
        }
        
        std::vector<std::vector<std::vector<uint16_t>>> *fastwfs = GetWaveforms(1742);
        
        for (unsigned int evt=0; evt < fastwfs->size(); evt++) {
            int SIC = fDGH->SIC[board_index][evt];
            
            for(unsigned int ch=0; ch<8; ch++) { //only first 8 channels - hard coded
                if((*channel_offsets)[ch]>-0.35 && (*channel_offsets)[ch]<-0.25){
                    std::vector<uint16_t> tmp((*fastwfs)[evt][ch]);
                    for(unsigned int samp=0; samp<1024; samp++) {
                        int sic_index = (samp+SIC)%1024;
                        tmp[samp]    -= (*table_cell)[ch][sic_index];
                        tmp[samp]    -= (*table_nsample)[ch][samp];
                    }
                    (*fastwfs)[evt][ch]=tmp;
                }
            }
            
            PeakCorrection((*fastwfs)[evt]);
        }
        
    }
    
    
    TMReaderInterface* OpenMidasFile(std::string filename) {
        TMReaderInterface* reader = TMNewReader(filename.c_str());
        if (reader->fError) {
            throw std::runtime_error("Cannot open "+filename+"\n");
            delete reader;
        }
        return reader;
    }
    bool FindODBDumpBOR(TMidasEvent &event, bool verbose) {
        if ((event.GetEventId() & 0xFFFF) == 0x8000) {
            if (verbose) printf("Event: this is a begin of run ODB dump\n");
            return true;
        }
        return false;
    }
    MVOdb* GetODBDumpBOR(TMidasEvent &event,  MVOdbError *odberror) {
        if ((event.GetEventId() & 0xFFFF) != 0x8000) {
            throw std::runtime_error("cygnolib::GetODBDumpBOR: event is not a begin of run ODB dump.");
        } 
        return MakeFileDumpOdb(event.GetData(),event.GetDataSize(), odberror);
    }
    
    bool FindBankByName(TMidasEvent &event, std::string bname, bool verbose) {
        if ((event.GetEventId() & 0xFFFF) == 0x8000) {
            if (verbose) printf("Event: this is a begin of run ODB dump\n");
            return false;
        } else if ((event.GetEventId() & 0xFFFF) == 0x8001) {
            if (verbose) printf("Event: this is an end of run ODB dump\n");
            return false;
        }
        
        event.SetBankList();
        int bankLength = 0;
        int bankType = 0;
        void *pdata = 0;
        bool found = event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
        return found;
    }
    
    
    
    void InitializePMTReadout(std::string filename,
                              bool *DRS4correction,
                              std::vector<float> *channels_offsets,
                              std::string tag,
                              std::vector<std::vector<int>> &table_cell,
                              std::vector<std::vector<int>> &table_nsample) {
        
        std::string filepath(getenv("RECOPPSYS"));
        
        if(tag!="LNGS" && tag!="LNF") {
            throw std::runtime_error("cygnolib::InitializePMTReadout: unknown tag "+tag+".\n");
        }
        
        std::string    table_cell_filename(filepath+"/input/table_cell_"   +tag+".txt");
        std::string table_nsample_filename(filepath+"/input/table_nsample_"+tag+".txt");
        std::string x;
        
        
        
        std::ifstream inFile;
        inFile.open(table_cell_filename);
        
        std::vector<std::vector<int>> ftable(8, std::vector<int>(1024, 0));
        int counter=0;
        while (!inFile.eof()) {
            std::vector<int> line_tmp(1024, 0);
            
            for(unsigned int i=0; i<1024; i++) {
                inFile>> x;
                line_tmp[i] = std::stoi(x);
            }
            if(counter<(int)ftable.size()) ftable[counter]=line_tmp;
            counter++;
        }
        inFile.close();
        table_cell = ftable;
        
        inFile.open(table_nsample_filename);
        std::vector<std::vector<int>> ftable2(8, std::vector<int>(1024, 0));
        counter=0;
        while (!inFile.eof()) {
            std::vector<int> line_tmp(1024, 0);
            for(unsigned int i=0; i<1024; i++) {
                inFile>> x;
                line_tmp[i] = std::stoi(x);
            }
            if(counter<(int)ftable2.size()) ftable2[counter]=line_tmp;
            counter++;
        }
        inFile.close();
        table_nsample = ftable2;
        
        TMReaderInterface* reader = cygnolib::OpenMidasFile(filename);

        bool reading = true;
        while(reading) {
            TMidasEvent event = TMidasEvent();
            reading = TMReadEvent(reader, &event);
            bool odb_bor_found = cygnolib::FindODBDumpBOR(event, true);
            if(odb_bor_found) {
                MVOdb* odb = NULL;
                odb = cygnolib::GetODBDumpBOR(event);


                odb->RB("/Configurations/DRS4Correction", DRS4correction);
                odb->RFA("/Configurations/DigitizerOffset", channels_offsets);

                reader->Close();
                break;
            }
        }
    }
    
    Picture daq_cam2pic(TMidasEvent &event, std::string cam_model) {
        int rows;
        int columns;
        
        if(cam_model=="fusion") {
            rows = 2304;
            columns = 2304;
        } else {
            throw std::invalid_argument("cygnolib::daq_cam2pic: invalid model '"+cam_model+"' for the camera.\n");
        }
        
        Picture pic(rows, columns);
        
        std::string bname="CAM0";
        int bankLength = 0;
        int bankType = 0;
        void *pdata = 0;
        event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
        uint16_t *pdatacast = (uint16_t *)pdata; //recast to bank type to increment
        std::vector<std::vector<uint16_t>> frame(rows, std::vector<uint16_t>(columns,0)); 
        for(int i=0; i<rows; i++) {
            for (int j=0; j<columns;j++) {
                frame[i][j] = *pdatacast;
                pdatacast++;
            }
        }
        pic.SetFrame(frame);
        return pic;
        
    }
    DGHeader daq_dgh2head(TMidasEvent &event) {
        std::string bname="DGH0";
        int bankLength = 0;
        int bankType = 0;
        void *pdata = 0;
        event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
        uint32_t *pdatacast = (uint32_t *)pdata; //recast to bank type to increment
        
        std::vector<uint32_t> v(bankLength, 0);
        for (int i=0; i<bankLength; i++) {
            v[i] = *pdatacast;
            pdatacast++;
        }
        
        DGHeader DGH(v);
        return DGH;
    }
    
    PMTData daq_dig2PMTData(TMidasEvent &event, DGHeader *DGH) {
        std::string bname="DIG0";
        int bankLength = 0;
        int bankType = 0;
        void *pdata = 0;
        event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
        uint16_t *pdatacast = (uint16_t *)pdata; //recast to bank type to increment
        
        std::vector<uint16_t> v(bankLength, 0);
        for (int i=0; i<bankLength; i++) {
            v[i] = *pdatacast;
            pdatacast++;
        }
        //return v;
        
        PMTData PMTD(DGH, v);
        return PMTD;
    }
    
}

