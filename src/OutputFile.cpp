/*
   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011 Her Majesty
   the Queen in Right of Canada (Communications Research Center Canada)

   Copyright (C) 2016
   Matthias P. Braendli, matthias.braendli@mpb.li

    http://opendigitalradio.org
 */
/*
   This file is part of ODR-DabMod.

   ODR-DabMod is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   ODR-DabMod is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with ODR-DabMod.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OutputFile.h"
#include "PcDebug.h"

#include <string>
#include <assert.h>
#include <stdexcept>


OutputFile::OutputFile(std::string filename) :
    ModOutput(),
    myFilename(filename)
{
    PDEBUG("OutputFile::OutputFile(filename: %s) @ %p\n",
            filename.c_str(), this);

    myFile = fopen(filename.c_str(), "w");
    if (myFile == NULL) {
        perror(filename.c_str());
        throw std::runtime_error(
                "OutputFile::OutputFile() unable to open file!");
    }
}


OutputFile::~OutputFile()
{
    PDEBUG("OutputFile::~OutputFile() @ %p\n", this);

    if (myFile != NULL) {
        fclose(myFile);
    }
}


int OutputFile::process(Buffer* dataIn)
{
    PDEBUG("OutputFile::process(%p)\n", dataIn);
    assert(dataIn != NULL);

    if (fwrite(dataIn->getData(), dataIn->getLength(), 1, myFile) == 0) {
        throw std::runtime_error(
                "OutputFile::process() unable to write to file!");
    }

    return dataIn->getLength();
}
