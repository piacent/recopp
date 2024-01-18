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


namespace cygnolib {
    
    Picture::Picture(int height, int width): nrows(height), ncolumns(width), frame(height, std::vector<uint16_t>(width,0)) {
    }
    Picture::~Picture(){
    }
    int Picture::GetNRows(){
        return nrows;
    }
    int Picture::GetNColumns(){
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
        for (int r = 0; r < nrows; ++r) {
            for (int c = 0; c < ncolumns; ++c) {
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
            
            
            if(rawheader.size()<baseidx+6+nchannels[i]+2*nwaveforms[i] && board_model[i] ==1742) {
                throw std::runtime_error("cygnolib::DGHeader::DGHeader: corrupted.");
            } else if(rawheader.size()<baseidx+6+nchannels[i]+nwaveforms[i] && board_model[i] ==1720) {
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
        nboards;
        std::cout<<"=========================="<<std::endl;
        for(unsigned int i=0;i<nboards;i++){
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
    
    Board::Board(DGHeader DGH, std::vector<uint16_t> rawwaveforms, int board_model):
    fDGH(DGH), fboard_model(board_model) {
        
        int nboards = fDGH.nboards;
        for(int i=0;i<nboards;i++) {
            if(fDGH.board_model[i]==fboard_model){
                fnchannels  = fDGH.nchannels[i];
                fnsamples   = fDGH.nsamples[i];
                fnwaveforms = fDGH.nwaveforms[i];
            }
        }
        
        if(rawwaveforms.size() != fnchannels*fnsamples*fnwaveforms) {
            throw std::runtime_error("cygnolib::Board::Board: input for board model "+
                                     std::to_string(fboard_model)+
                                     " has wrong dimensions.");
        }
        
        std::vector<std::vector<std::vector<uint16_t>>> tmp(fnwaveforms,
                                                            std::vector<std::vector<uint16_t>>(fnchannels, 
                                                            std::vector<uint16_t>(fnsamples, 0)
                                                            )
                                                           );
        
        for(int evt=0; evt<fnwaveforms; evt++) {
            for (int ch=0; ch<fnchannels; ch++) {
                for (int samp=0; samp<fnsamples; samp++) {
                    int index = evt*fnchannels*fnsamples + ch*fnsamples + samp;
                    tmp[evt][ch][samp] = rawwaveforms[index];
                }
            }
        }
        
        fData = tmp;
        
    }
    Board::~Board(){
    }
    std::vector<std::vector<std::vector<uint16_t>>> Board::GetData() {
        return fData;
    }
    
    std::list<Board> PMTData::data; 
    PMTData::PMTData(DGHeader DGH, std::vector<uint16_t> rawwaveforms): fDGH(DGH){
        int nboards = fDGH.nboards;
        
        int baseidx = 0;
        
        unsigned int totlength = 0;
        for(int i=0;i<nboards;i++) {
            int length_i = fDGH.nchannels[i]*fDGH.nsamples[i]*fDGH.nwaveforms[i];
            totlength += length_i;
        }
        if(totlength!=rawwaveforms.size()) {
            throw std::runtime_error("cygnolib::PMTData::PMTdata: corrupted.");
        }
        
        
        for(int i=0;i<nboards;i++) {
            int length_i = fDGH.nchannels[i]*fDGH.nsamples[i]*fDGH.nwaveforms[i];
            std::vector<uint16_t> rawwaveforms_i;
            for(int j=0; j<length_i; j++) {
                rawwaveforms_i.push_back(rawwaveforms[baseidx+j]);
            }
            baseidx += length_i;
            
            Board board_i(fDGH, rawwaveforms_i, fDGH.board_model[i]);
            
            data.push_back(board_i);
        }
    }
    PMTData::~PMTData(){
    }
    std::vector<std::vector<std::vector<uint16_t>>> PMTData::GetWaveforms(int board_model) {
        int nboards = fDGH.nboards;
        bool board_found = false;
        int board_index = -1;
        for(int i=0;i<nboards;i++) {
            if(board_model==fDGH.board_model[i]) {
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
        
        return (*data_front).GetData();
    }
    
    
    void foo(){
        std::cout<<"Hello inside"<<std::endl;
        
        std::string filename="/data11/cygno/piacenst/stefano/cygnocpp/debug/run35138.mid.gz";
        
        std::string bname="CAM0";
        
        int counter = 0;
        MVOdb* odb = NULL;
        MVOdbError odberror;
        
        TMReaderInterface* reader = TMNewReader(filename.c_str());
        if (reader->fError) {
            std::cout<<"Cannot open input file "<<filename.c_str()<<std::endl;
            delete reader;
        }
        
        
        while (1) {
            //TMEvent* e = TMReadEvent(reader);
            TMidasEvent event = TMidasEvent();
            bool reading = TMReadEvent(reader, &event);
            
            if (!reading) {
                // EOF
                std::cout<<"EOF reached."<<std::endl;
                break;
            }
            
            /*printf("Event: id 0x%04x, mask 0x%04x, serial 0x%08x, time 0x%08x, data size %d\n",
                   event.GetEventId(),
                   event.GetTriggerMask(),
                   event.GetSerialNumber(),
                   event.GetTimeStamp(),
                   event.GetDataSize()
                  );*/
            
            
            if ((event.GetEventId() & 0xFFFF) == 0x8000) {
                printf("Event: this is a begin of run ODB dump\n");
                odb = MakeFileDumpOdb(event.GetData(),event.GetDataSize(), &odberror);
                continue;
            } else if ((event.GetEventId() & 0xFFFF) == 0x8001) {
                printf("Event: this is an end of run ODB dump\n");
                odb = MakeFileDumpOdb(event.GetData(),event.GetDataSize(), &odberror);
                continue;
            }

            //
            int N = event.SetBankList();
            /*for (int i = 0; i < N * 4; i += 4) {
                int bankLength = 0;
                int bankType = 0;
                void *pdata = 0;
                int found = event.FindBank(&(event.GetBankList()[i]), &bankLength, &bankType, &pdata);
                printf("Bank %c%c%c%c, length %6d, type %2d\n",
                       event.GetBankList()[i],
                       event.GetBankList()[i+1],
                       event.GetBankList()[i+2],
                       event.GetBankList()[i+3],
                       bankLength,
                       bankType
                      );
            }*/
//             for (int i = 0; i < N * 4; i += 4) {
            int bankLength = 0;
            int bankType = 0;
            void *pdata = 0;
            int found = event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
            uint16_t *pdatacast = (uint16_t *)pdata; //recast to bank type to increment
            
            date::sys_seconds tp{std::chrono::seconds{event.GetTimeStamp()}};
            std::string s = date::format("%Y-%m-%d %I:%M:%S %p", tp);
            std::uint32_t banksize = event.GetDataSize();
            
            int rows     = 2304;
            int columns  = 2304;
            
            std::vector<std::vector<uint16_t>> frame(rows, std::vector<uint16_t>(columns,0)); 
            
            if(found) {
                std::cout<<"FOUND "<<bname<<" bank"<<std::endl;
                std::cout<<"Event: id "<<event.GetEventId()<<std::endl
                         <<"     mask "<<event.GetTriggerMask()<<std::endl
                         <<"   serial "<<event.GetSerialNumber()<<std::endl
                         <<"     time "<<s<<std::endl
                         <<" datasize "<<banksize<<std::endl
                         <<std::endl;
                
                std::cout<<"Pixels = "<<sqrt(bankLength)<<std::endl;
                
                if(event.GetSerialNumber() == 0){
                    for(int i=0; i<rows; i++) {
                        for (int j=0; j<columns;j++) {
                            frame[i][j] = *pdatacast;
                            *pdatacast++;
                        }
                    }
                    
                    std::cout<<frame[0][0]<<","<<frame[0][1]<<","<<frame[0][2]<<","<<std::endl;
                    std::cout<<frame[1][0]<<","<<frame[1][1]<<","<<frame[1][2]<<","<<std::endl;
                    std::cout<<frame[2][0]<<","<<frame[2][1]<<","<<frame[2][2]<<","<<std::endl;
                }
                
                
            }
            
            
            
            
            counter++;
         }
         
         reader->Close();
         delete reader;
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
        
        int N = event.SetBankList();
        int bankLength = 0;
        int bankType = 0;
        void *pdata = 0;
        bool found = event.FindBank("CAM0", &bankLength, &bankType, &pdata);
        return found;
    }
    
    
    
    void InitializePMTReadout(std::string filename, bool *DRS4correction, std::vector<float> *channels_offsets) {
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
        int found = event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
        uint16_t *pdatacast = (uint16_t *)pdata; //recast to bank type to increment
        std::vector<std::vector<uint16_t>> frame(rows, std::vector<uint16_t>(columns,0)); 
        for(int i=0; i<rows; i++) {
            for (int j=0; j<columns;j++) {
                frame[i][j] = *pdatacast;
                *pdatacast++;
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
        int found = event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
        uint32_t *pdatacast = (uint32_t *)pdata; //recast to bank type to increment
        
        std::vector<uint32_t> v(bankLength, 0);
        for (int i=0; i<bankLength; i++) {
            v[i] = *pdatacast;
            *pdatacast++;
        }
        
        DGHeader DGH(v);
        return DGH;
    }
    
    PMTData daq_dig2PMTData(TMidasEvent &event, DGHeader DGH) {
        std::string bname="DIG0";
        int bankLength = 0;
        int bankType = 0;
        void *pdata = 0;
        int found = event.FindBank(bname.c_str(), &bankLength, &bankType, &pdata);
        uint16_t *pdatacast = (uint16_t *)pdata; //recast to bank type to increment
        
        std::vector<uint16_t> v(bankLength, 0);
        for (int i=0; i<bankLength; i++) {
            v[i] = *pdatacast;
            *pdatacast++;
        }
        //return v;
        
        PMTData PMTD(DGH, v);
        return PMTD;
    }
    
}

