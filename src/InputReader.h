/*
   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
   Her Majesty the Queen in Right of Canada (Communications Research
   Center Canada)

   Copyright (C) 2018
   Matthias P. Braendli, matthias.braendli@mpb.li

    http://www.opendigitalradio.org
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

#pragma once

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <cstdio>
#include <vector>
#include <atomic>
#include <memory>
#if defined(HAVE_ZEROMQ)
#  include "zmq.hpp"
#  include "ThreadsafeQueue.h"
#endif
#include "porting.h"
#include "Log.h"
#include <unistd.h>
#define INVALID_SOCKET   -1

class InputReader
{
    public:
        // Put next frame into buffer. This function will never write more than
        // 6144 bytes into buffer.
        // returns number of bytes written to buffer, 0 on eof, -1 on error
        virtual int GetNextFrame(void* buffer) = 0;

        // Print some information
        virtual void PrintInfo() const = 0;
};

class InputFileReader : public InputReader
{
    public:
        InputFileReader() = default;
        InputFileReader(const InputFileReader& other) = delete;
        InputFileReader& operator=(const InputFileReader& other) = delete;

        // open file and determine stream type
        // When loop=1, GetNextFrame will never return 0
        int Open(std::string filename, bool loop);

        // Print information about the file opened
        void PrintInfo() const;
        int GetNextFrame(void* buffer);

    private:
        int IdentifyType();

        // Rewind the file, and replay anew
        // returns 0 on success, -1 on failure
        int Rewind();

        bool loop_; // if shall we loop the file over and over
        std::string filename_;

        /* Known types of input streams. Description taken from the CRC
         * mmbTools forum. All values are are little-endian.  */
        enum class EtiStreamType {
            /* Not yet identified */
            None,

            /* Raw format is a bit-by-bit (but byte aligned on sync) recording
             * of a G.703 data stream. The padding is always present.
             * The raw format can also be referred to as ETI(NI, G.703) or ETI(NI).
             * Format:
                 for each frame:
                   uint8_t data[6144]
             */
            Raw,

            /* Streamed format is used for streamed applications. As the total
             * number of frames is unknown before end of transmission, the
             * corresponding field is removed. The padding can be removed from
             * data.
             * Format:
                 for each frame:
                   uint16_t frameSize
                   uint8_t data[frameSize]
             */
            Streamed,

            /* Framed format is used for file recording. It is the default format.
             * The padding can be removed from data.
             * Format:
                 uint32_t nbFrames
                 for each frame:
                   uint16_t frameSize
                   uint8_t data[frameSize]
             */
            Framed,
        };

        EtiStreamType streamtype_ = EtiStreamType::None;
        struct FILEDeleter{ void operator()(FILE* fd){ if(fd) fclose(fd);}};
        std::unique_ptr<FILE, FILEDeleter> inputfile_;

        size_t inputfilelength_ = 0;
        uint64_t nbframes_ = 0; // 64-bit because 32-bit overflow is
        // after 2**32 * 24ms ~= 3.3 years
};

class InputTcpReader : public InputReader
{
    public:
        InputTcpReader();
        InputTcpReader(const InputTcpReader& other) = delete;
        InputTcpReader& operator=(const InputTcpReader& other) = delete;
        virtual ~InputTcpReader();

        // Endpoint is either host:port or tcp://host:port
        void Open(const std::string& endpoint);

        // Put next frame into buffer. This function will never write more than
        // 6144 bytes into buffer.
        // returns number of bytes written to buffer, 0 on eof, -1 on error
        virtual int GetNextFrame(void* buffer);

        // Print some information
        virtual void PrintInfo() const;

    private:
        int m_sock = INVALID_SOCKET;
        std::string m_uri;
};

struct zmq_input_overflow : public std::exception
{
    const char* what () const throw ()
    {
        return "InputZMQ buffer overflow";
    }
};

#if defined(HAVE_ZEROMQ)
/* A ZeroMQ input. See www.zeromq.org for more info */

class InputZeroMQReader : public InputReader
{
    public:
        InputZeroMQReader() = default;
        InputZeroMQReader(const InputZeroMQReader& other) = delete;
        InputZeroMQReader& operator=(const InputZeroMQReader& other) = delete;
        ~InputZeroMQReader();

        int Open(const std::string& uri, size_t max_queued_frames);
        int GetNextFrame(void* buffer);
        void PrintInfo() const;

    private:
        std::atomic<bool> m_running = ATOMIC_VAR_INIT(false);
        std::string m_uri;
        size_t m_max_queued_frames = 0;
        ThreadsafeQueue<std::shared_ptr<std::vector<uint8_t> > > m_in_messages;

        void RecvProcess(void);

        zmq::context_t m_zmqcontext; // is thread-safe
        std::thread m_recv_thread;

        /* We must be careful to keep frame phase consistent. If we
         * drop a single ETI frame, we will break the transmission
         * frame vs. ETI frame phase.
         *
         * Here we keep track of how many ETI frames we must drop.
         */
        int m_to_drop = 0;
};

#endif

