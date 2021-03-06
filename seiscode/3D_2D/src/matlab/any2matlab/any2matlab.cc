/*!\file any2matlab.cc
 * \brief This es a MEX-file using Thomas Forbriger's datrwxx library importing
 * seismic dataformats to MATLAB.
 *
 * \author Daniel Armbruster
 * \version V1.2
 * \date 07/09/2011
 *
 * ----
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * ----
 *
 * REVISIONS and CHANGES 
 *  - 01/07/2010  V1.0    Daniel Armbruster
 *  - 30/09/2010  V1.1    introduced option 'int'
 *  - 30/04/2011  V1.1.1  bug fixing and addaption of documentation to the new
 *                        package mechanism
 *  - 07/09/2011  V1.2    support format modifier strings
 *  - 31/10/2013  V1.3    (Thomas Forbriger): removed explicit list of format
 *                        types since this was out of date and always is
 *                        likely to be out of date
 *
 * \section general General
 *
 * - For information how to use go to \ref usage .
 * - For \ref info how the program works.
 *
 * \brief The gateway routine to every MEX-file is called <b>mexFunction</b>.
 *
 * \param nrhs: Number of inputs (Right Hand Side)
 * \param prhs: Array of pointers to input data. 
 *
 * The input data is read-only and should not be altered by your mexFunction  .
 * \return nlhs Number of expected mxArrays (Left Hand Side)  
 * \return plhs: Array of pointers to expected outputs
 *
 * The variables nrhs and \c nlhs are the number of variables that MATLAB requested at this instance.
 * They are analogous to \c NARGIN and \c NARGOUT in MATLAB.
 * \n
 *
 * The variables \c prhs and \c plhs are not mxArrays. They are arrays of pointers to mxArrays. So if a
 * function is given three inputs, \c prhs  will be an array of three pointers to the mxArrays that
 * contain the data passed in. The variable \c prhs is declared as \c const. This means that the values
 * that are passed into the MEX-file should not be altered. Doing so can cause segmentation
 * violations in MATLAB. The values in \c plhs are invalid when the MEX-file begins. The mxArrays they
 * point to must be explicitly created before they are used. Compilers will not catch this issue,
 * but it will cause incorrect results or segmentation violations.
 *
 * \section usage Usage
 *     <tt>tfstruct = any2matlab('name.ftype');\n</tt>
 * or: <tt>tfstruct = any2matlab('name', 'ftype');\n</tt>
 * or: <tt>tfstruct = any2matlab('name', 'ftype', 'dtype');\n</tt>
 * 
 * - \p name is the filename of the datafile. A "." seperates \p name from \p ftype
 * e.g. \p data.sff
 * - \p name is the filename of the datafile in an arbitrary form. \p ftype is one of the options
 * below.
 * - \p name is the filename of the datafile in an arbitrary form. \p ftype is one of the options
 * below. \p dtype is the type the data will be imported.\n\n 
 * - \p dtype is one of the following two options
 * -# \c int for less memory use
 * -# \c double as default
 * -# \b Attention: The integer data is generated by cutting the decimals of the double data.
 *  Due to rounding problems on computers it can occur, that a double number of 9.999*10^1 will be casted to 0 (zero).
 * - \p ftype is one the file types ID as used in libdatrwxx
 * - \p tfstruct is a MATLAB struct with the following fields:
 * -# \c date: date of first sample. format: \c 'YYYY/MM/DD'
 * -# \c time: time of the first sample. format: \c 'hh:mm:ss.milsec'
 * -# \c tdate: timevector: <tt>[YYYY MM DD hh mm ss.milsec]</tt>
 * -# \c station: Station code. type: \c string
 * -# \c channel: FDSN channel code. type: \c string
 * -# \c auxid: Auxiliary identification code. type: \c string
 * -# \c samps: Number of samples. type: \c int
 * -# \c dt: Sampling interval (sec). type: \c double 
 * -# \c calib: Calibration factor. type: \c double
 * -# \c calper: Calibration reference period. type: \c double
 * -# \c instype: Instrument type. type: \c string
 * -# \c trace: [<tt>samps</tt>x1 \c double] or [<tt>samps</tt>x1 \c int] 
 *
 * For further information watch sff::WID2
 *
 * \section info Detailed Information
 *
 * To allocate sufficient memory any2matlab has to open a first time the desired datafile to get the
 * number of traces and the number of samps per trace.\n
 * While opening the datafile a second time any2matlab will read one trace and write it immediately
 * to \c tfstruct until there are no traces left.\n
 * With this method you are able to read as much traces as fit to your machines memory.
 *
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <ctype.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <vector>
#include <datrwxx/readany.h>
#include <libtime++.h>
#include <aff/series.h>
#include <gsexx.h>
#include <sffxx.h>
#include "mex.h"

//! mexFunction of any2matlab to read seismic data to MATLAB
/*!
    \param nlhs number of return arguments
    \param plhs the return arguments
    \param nrhs number of input arguments
    \param prhs the input arguments
*/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

  const char *field_names[] = {"date", "time", "tdate", "station", "channel", "auxid", "samps", "dt",
"calib", "calper", "instype", "trace"};
  int number_of_fields;
  int date_field, time_field, tdate_field, station_field, channel_field;
  int auxid_field, samps_field, dt_field, calib_field, calper_field;
  int instype_field, trace_field;
  std::vector<int> ntraces;
  ntraces.reserve(100);

  // calculate number of fields
  number_of_fields = sizeof(field_names) / sizeof(*field_names);

  // check: if input is correct 
  if (nrhs == 0)
    mexErrMsgTxt("ERROR: missing input filename!");
  // check: more than 2 arguments
  else if (nrhs > 3)
    mexErrMsgTxt("ERROR: too many input arguments!");
  // check: one argument and string
  else if ((nrhs == 1) && !mxIsChar(prhs[0])) 
    mexErrMsgTxt("ERROR: input value must be a string!");
  // check: 2 arguments and string 
  else if (nrhs == 2 && (!mxIsChar(prhs[0]) || !mxIsChar(prhs[1])))
    mexErrMsgTxt("Error: input values must be strings!");
  // input must be a row vectors 
  else if (nrhs == 2 && ((mxGetM(prhs[0]) != 1) || mxGetM(prhs[1]) != 1))
    mexErrMsgTxt("Error: input isn't valid!");
  else if (nrhs == 3 && (!mxIsChar(prhs[0]) || !mxIsChar(prhs[1]) || !mxIsChar(prhs[2])))
    mexErrMsgTxt("Error: input values must be strings!");
  else if (nrhs == 3) {
    for(int i = 0; i < 3; ++i) {
      if(mxGetM(prhs[i]) != 1) 
        mexErrMsgTxt("Error: input isn't valid!");
    }
  }

  std::string infile;
  std::string filetype;

  // If only one argument is given to any2matlab.mexa64
  if (nrhs == 1) {
    infile = std::string(mxArrayToString(prhs[0]));

  /*
	* Important note:
	* If only one argument is given to any2matlab.mexa64 the given filename must
  * contain at least one dot. After the the dot there must be one of the
  * following endings.
	*/ 
	if (nrhs == 1 && infile.rfind(".") != string::npos) { 
      filetype = infile.substr(infile.rfind(".") + 1);
	 }
	 else  
      mexErrMsgTxt("Error: not a valid filename!");
  }

  // check: 2 arguments
  if (nrhs == 2) {
    infile = std::string(mxArrayToString(prhs[0]));
    filetype = std::string(mxArrayToString(prhs[1]));
  }

  // set format to default value: double
  std::string dtype("double");

  // check: 3 arguments
  if (nrhs == 3) {
    infile = std::string(mxArrayToString(prhs[0]));
    filetype = std::string(mxArrayToString(prhs[1]));

    std::string arg2(mxArrayToString(prhs[2]));
    // check: third argument
	  if (arg2 == "int" || arg2 == "double") { 
      dtype = arg2;
	  }
	  else {
      mexErrMsgTxt("Error: third argument is not valid!");
    }
  }

  // set filetype to lower case
  transform(filetype.begin(), filetype.end(), filetype.begin(), tolower);
  int trace = 0;
  bool hot = false;

  {	 
    std::ifstream ifs(infile.c_str(), datrw::ianystream::openmode(filetype));
    datrw::ianystream is(ifs, filetype);
 
    /* get number of traces in infile */
    /* while skipping series */
    hot=is.good();

    while (hot) {
      is.skipseries();

      sff::WID2 wid2;
      // read the WID2 header
      is >> wid2;
    
      ntraces.push_back(wid2.nsamples);
      //mexPrintf("samples: %d\n", wid2.nsamples);

      trace++;

      hot=(!is.last());
    }
  }

  int dims[2] = {1, trace};
  // create a 1-by-n array of strucntraces.  
  plhs[0] = mxCreateStructArray(2, dims, number_of_fields, field_names);

  date_field = mxGetFieldNumber(plhs[0], "date");
  time_field = mxGetFieldNumber(plhs[0], "time");
  tdate_field = mxGetFieldNumber(plhs[0], "tdate");
  station_field = mxGetFieldNumber(plhs[0], "station");
  auxid_field = mxGetFieldNumber(plhs[0], "auxid");
  channel_field = mxGetFieldNumber(plhs[0], "channel");
  samps_field = mxGetFieldNumber(plhs[0], "samps");
  dt_field = mxGetFieldNumber(plhs[0], "dt");
  calib_field = mxGetFieldNumber(plhs[0], "calib");
  calper_field = mxGetFieldNumber(plhs[0], "calper");
  instype_field = mxGetFieldNumber(plhs[0], "instype");
  trace_field = mxGetFieldNumber(plhs[0], "trace");

  {
    std::ifstream ifs(infile.c_str(), datrw::ianystream::openmode(filetype));
    datrw::ianystream is(ifs, filetype);
    hot = is.good();

    int i = 0;

    while(hot) {
      mxArray *field_value;
      
      // read integer data
      if (dtype == "int") {
        datrw::Tiseries iseries;

        // read the series
        is >> iseries;

        // set trace-field
        field_value = mxCreateNumericMatrix(ntraces[i], 1, mxINT32_CLASS,
          mxREAL);
        // copy stuff
        int *p = iseries.pointer();
        int *ptr = static_cast<int*>(mxGetData(field_value));
        for(int j = 0; j < iseries.size(); ++j) {
          *ptr = *p;
          p++;
          ptr++;
        }
        mxSetFieldByNumber(plhs[0], i, trace_field, field_value);
      }
      // read double data - default
      else {
        datrw::Tdseries dseries;
        // read the series
        is >> dseries;

        // set trace-field
        field_value = mxCreateNumericMatrix(ntraces[i], 1, mxDOUBLE_CLASS,
          mxREAL);
        // copy stuff
        double *ptr = mxGetPr(field_value); 
        double *tmp = dseries.pointer();
        for(int j = 0; j < dseries.size(); ++j) {
          ptr[j] = *tmp;
          tmp++;
        }
        mxSetFieldByNumber(plhs[0], i, trace_field, field_value);
      }

      sff::WID2 wid2;
      // read the WID2 header
      is >> wid2;

      // set date-field
      {
        std::ostringstream str;
        str << wid2.date.year() << "/" 
          << wid2.date.month() << "/" 
          << wid2.date.day() << std::ends;
        std::string datestring; 
        datestring.append(str.str());
        mxSetFieldByNumber(plhs[0], i, date_field,
          mxCreateString(datestring.c_str()));
      }

      // set date-field
      {
        std::ostringstream str;
        std::string timestring; 
        str << wid2.date.hour() << ":" 
          << wid2.date.minute() << ":" 
          << wid2.date.second() << "." 
          << wid2.date.milsec() << std::ends;
        timestring.append(str.str());
        mxSetFieldByNumber(plhs[0], i, time_field,
          mxCreateString(timestring.c_str()));
      }

      // set tdate-field
      double time [6];
      time [0] = wid2.date.year();
      time [1] = wid2.date.month();
      time [2] = wid2.date.day();
      time [3] = wid2.date.hour();
      time [4] = wid2.date.minute();
      time [5] = wid2.date.second() + wid2.date.milsec() * 0.001;

      field_value = mxCreateDoubleMatrix(1,
        sizeof(time) / sizeof(time[0]), mxREAL);
      memmove(mxGetPr(field_value), &time,
        sizeof(time) / sizeof(time[0]) * sizeof(double));
      mxSetFieldByNumber(plhs[0], i, tdate_field, field_value);

      // set station-field
      mxSetFieldByNumber(plhs[0], i, station_field,
        mxCreateString(wid2.station.c_str()));

      // set channel-field
      mxSetFieldByNumber(plhs[0], i, channel_field,
        mxCreateString(wid2.channel.c_str()));

      // set auxid-field
      mxSetFieldByNumber(plhs[0], i, auxid_field,
        mxCreateString(wid2.auxid.c_str()));

      // set samps-field
      field_value = mxCreateDoubleMatrix(1, 1, mxREAL);
      *mxGetPr(field_value) = wid2.nsamples;
      mxSetFieldByNumber(plhs[0], i, samps_field, field_value);

      // set dt-field
      field_value = mxCreateDoubleMatrix(1, 1, mxREAL);
      *mxGetPr(field_value) = wid2.dt;
      mxSetFieldByNumber(plhs[0], i, dt_field, field_value);

      // set calib-field
      field_value = mxCreateDoubleMatrix(1, 1, mxREAL);
      *mxGetPr(field_value) = wid2.calib;
      mxSetFieldByNumber(plhs[0], i, calib_field, field_value);

      // set calper-field
      field_value = mxCreateDoubleMatrix(1, 1, mxREAL);
      *mxGetPr(field_value) = wid2.calper;
      mxSetFieldByNumber(plhs[0], i, calper_field, field_value);

      // set instype-field
      mxSetFieldByNumber(plhs[0], i, instype_field,
        mxCreateString(wid2.instype.c_str()));

      i++;
      hot=(!is.last());
    }
  }
}

