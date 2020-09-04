/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __GPSNMEA_V2_H__
#define __GPSNMEA_V2_H__

#include "config.h"

#include "kis_gps.h"
#include "timetracker.h"
#include "buffer_handler.h"
#include "ringbuf2.h"
#include "globalregistry.h"
#include "serialclient2.h"
#include "pollabletracker.h"

#define ASIO_HAS_STD_CHRONO
#define ASIO_HAS_MOVE

#include "asio.hpp"

// Generic NMEA parser for GPS

class kis_gps_nmea_v2 : public kis_gps {
public:
    kis_gps_nmea_v2(shared_gps_builder in_builder) :
        kis_gps(in_builder) { }

    virtual ~kis_gps_nmea_v2() { };

protected:
    virtual void close() = 0;

    virtual void start_read();
    virtual void start_read_impl() = 0;
    virtual void handle_read(const std::error_code& error, std::size_t sz);

    asio::streambuf in_buf;
    std::atomic<bool> stopped;

    // Have we ever seen data from the device?
    bool ever_seen_gps;

    // Last time we calculated the heading, don't do it more than once every 
    // few seconds or we get nasty noise
    time_t last_heading_time;
};

#endif

