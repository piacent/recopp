/*
 * Copyright (C) 2024 CYGNO Collaboration
 *
 *
 * Author: Stefano Piacentini
 * Created in 2024
 *
 */

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
#include <numeric>


/**
 * @brief Cygnolib classes, functions, and implementations
 *
 *
 */
namespace cygnolib {
    
    
    /**
     * @class Picture
     * @brief A class for providing tools to handle the pictures collected by the CYGNO cameras
     * @author CYGNO Collaboration
     *
     * @details This class can be used to create an object that contains an image collected by a cygno
     * camera
     * 
     */
    class Picture {
    public:
        
        /**
         * @brief Constructor.
         * @details This constructor passes the dimensions of the image in pixels.
         *
         * @param[in] heigth Height of the image in pixel. Default value is 2304.
         * @param[in] width Width of the image in pixel. Default value is 2304.
         *
         */
        Picture(unsigned int height = 2304, unsigned int width = 2304);
        
        /**
         * @brief The default destructor.
         *
         */
        ~Picture();
        
        /**
         * @brief This method returns the number of rows of the image
         *
         * @return the number of rows of the image
         */
        unsigned int GetNRows();
        
        /**
         * @brief This method returns the number of columns of the image
         *
         * @return the number of columns of the image
         */
        unsigned int GetNColumns();
        
        /**
         * @brief This method returns the image in the form of a 2D std::vector
         *
         * @return the image
         */
        std::vector<std::vector<uint16_t>> GetFrame();
        
        /**
         * @brief This method sets the image
         *
         * @param[in] inputframe the image in the form og a 2D std::vector
         *
         */
        void SetFrame(std::vector<std::vector<uint16_t>> inputframe);
        
        /**
         * @brief This method prints a crop [0, a]x[0, b] of the image on stdout
         *
         * @param[in] a number of rows to print
         * @param[in] b number of columns to print
         *
         */
        void Print(int a, int b);
        
        /**
         * @brief This method saves the image on a file in grayscale
         *
         * @param[in] filename name of the output file
         * @param[in] vmin minimum intensity for the grayscale. Default value is 99.
         * @param[in] vmax maximum intensity for the grayscale. Default value is 130.
         *
         */
        void SavePng(std::string filename, int vmin = 99, int vmax = 130);
        
    private:
        unsigned int nrows;
        unsigned int ncolumns;
        std::vector<std::vector<uint16_t>> frame;
    };
    
    /**
     * @class DGHeader
     * @brief A class for providing tools to handle the Digitizer header collected by the CYGNO DAQ
     * @author CYGNO Collaboration
     *
     * @details This class can be used to create an object that contains the digitizer header of a
     * MIDAS event
     * 
     */
    class DGHeader {
    public:
        
        /**
         * @brief Constructor.
         * @details This constructor passes the raw header as contained in the MIDAS file.
         *
         * @param[in] rawheader the raw header contained in the bank DGH0 of the MIDAS file.
         *
         */
        DGHeader(std::vector<uint32_t> rawheader);
        
        /**
         * @brief The default destructor.
         *
         */
        ~DGHeader();
        
        /**
         * @brief This method prints the content of the digitizer header
         *
         */
        void Print();
        
        
        int nboards; ///< Number of boards
        std::vector<int> board_model; ///< models of the boards
        std::vector<int> nwaveforms;  ///< number of triggered waveforms acquired by the two boards
        std::vector<int> nchannels;   ///< number of channels of the boards
        std::vector<int> nsamples;    ///< number of samples of the boards
        std::vector<int> vertical_resolution;  ///< vertical resolution of the boards
        std::vector<int> sampling_rate;        ///< sampling rate of the boards
        std::vector<std::vector<int>> offsets; ///< DC offset of the boards
        std::vector<std::vector<int>> TTT;     ///< Trigger Time Tag of every triggered waveform
        std::vector<std::vector<int>> SIC;     ///< Start Index Cell of every triggered waveform
        
    };
    
    
    /**
     * @class PMTData
     * @brief A class for providing tools to handle the PMT data collected by the CYGNO DAQ
     * @author CYGNO Collaboration
     *
     * @details This class can be used to create an object that contains the data collected by the
     * PMTs and saved in the MIDAS event
     * 
     */
    class PMTData {
    public:
        
        /**
         * @brief Constructor.
         * @details This constructor passes the raw PMT data as contained in the MIDAS file.
         *
         * @param[in] rawwaveforms the raw PMT data contained in the bank DIG0 of the MIDAS file.
         *
         */
        PMTData(DGHeader *DGH, std::vector<uint16_t> rawwaveforms);
        /**
         * @brief The default destructor.
         *
         */
        ~PMTData();
        
        /**
         * @brief This method returns a pointer to the triggered PMT waveforms of the specified board
         *
         * @param[in] board_model an integer specifiying the board model
         *
         * @return a pointer to the triggered PMT waveforms
         */
        std::vector<std::vector<std::vector<uint16_t>>> *GetWaveforms(int board_model);
        
        /**
         * @brief This method applies the PeakCorrection to the raw waveforms collected.
         *
         * @details This method is developed and tested only for the board V1742. The implementation
         * is not yet general enough to be used also for other models. This correction
         * must be applied only after the 'cell' and 'nsample' correction.
         *
         * @param[in] wfs reference to the triggered waveforms data
         *
         */
        void PeakCorrection(std::vector<std::vector<uint16_t>> &wfs);
        
        /**
         * @brief This method applies the DRS4Corrections to the raw waveforms collected.
         *
         * @details This method is developed and tested only for the board V1742. The implementation
         * is not yet general enough to be used also for other models.
         *
         * @param[in] channels_offsets pointer to a std::vector containing the channel offsets
         * @param[in] table_cell pointer to the 'cell' correction tables
         * @param[in] table_nsample pointer to the 'nsample' correction tables
         *
         */
        void ApplyDRS4Corrections(std::vector<float> *channels_offsets,
                                  std::vector<std::vector<int>> *table_cell,
                                  std::vector<std::vector<int>> *table_nsample);
        
        
    private:
        std::list<std::vector<std::vector<std::vector<uint16_t>>>> data;
        DGHeader *fDGH;
        bool fCorrected;
        
        bool fCorrecting = false;
        
    };
    
    
    /**
     * @brief This function opens the MIDAS file
     *
     *
     * @param[in] filename name of the MIDAS file
     *
     */
    TMReaderInterface* OpenMidasFile(std::string filename);
    
    /**
     * @brief This function finds a MIDAS bank by its name
     *
     *
     * @param[in] event reference to MIDAS event
     * @param[in] bname name of the bank
     * @param[in] verbose flag for verbose mode
     *
     * @return a flag which is true if the event contains the bank, false otherwise
     *
     */
    bool FindBankByName(TMidasEvent &event, std::string bname, bool verbose = false);
    
    /**
     * @brief This function finds a MIDAS BOR (Begin Of Run) ODB dump
     *
     *
     * @param[in] event reference to MIDAS event
     * @param[in] verbose flag for verbose mode
     *
     * @return a flag which is true if the BOR ODB dump is found, false otherwise
     *
     */
    bool FindODBDumpBOR(TMidasEvent &event, bool verbose = false);
    
    /**
     * @brief This function finds a MIDAS BOR (Begin Of Run) ODB dump
     *
     *
     * @param[in] event reference to MIDAS event
     * @param[in] verbose flag for verbose mode
     *
     * @returns a pointer to the odb dump
     *
     */
    MVOdb* GetODBDumpBOR(TMidasEvent &event,  MVOdbError *odberror = NULL);
    
    
    /**
     * @brief This function initializes the PMT readout
     *
     * @param[in] filename name of the MIDAS file
     * @param[in] DRS4correction pointer to the DRS4Correction flag
     * @param[in] tag tag of the DAQ where the data have been collected (LNGS, LNF, ...)
     * @param[in] table_cell reference to the 'cell' correction table
     * @param[in] table_nsample reference to the 'nsample' correction table
     *
     */
    void InitializePMTReadout(std::string filename,
                              bool *DRS4correction, 
                              std::vector<float> *channels_offsets,
                              std::string tag,
                              std::vector<std::vector<int>> &table_cell,
                              std::vector<std::vector<int>> &table_nsample
                             );
    
    /**
     * @brief This function extracts the picture from the MIDAS event and converts it
     * to a Picture object
     *
     * @param[in] event reference to the MIDAS event
     * @param[in] cam_model model of the Hamamatsu camera
     *
     * @return the image as a Picture object
     *
     */
    Picture  daq_cam2pic(TMidasEvent &event, std::string cam_model = "fusion");
    
    /**
     * @brief This function extracts the digitizer header from the MIDAS event and
     * converts it to a DGHeader object
     *
     * @param[in] event reference to the MIDAS event
     *
     * @return the digitizer header as a DGHeader object
     *
     */
    DGHeader daq_dgh2head(TMidasEvent &event);
    
    /**
     * @brief This function extracts the PMT data from the MIDAS event and
     * converts it to a PMTData object
     *
     * @param[in] event reference to the MIDAS event
     *
     * @return the PMT data as a PMTData object
     *
     */
    PMTData daq_dig2PMTData(TMidasEvent &event, DGHeader *DGH);
    
    
}

#endif
