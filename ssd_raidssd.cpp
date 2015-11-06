/* Copyright (c) of KAIST, 2015 */
/*
* SRSim : A simulator for SSD-based RAID (Version 1.0)

* Author : HooYoung Ahn
* Class name : ssd_raidssd.cpp
* Description :This class creates multiple SSDs and gets read/write events from main programs
 */

/* FlashSim is free software: you can redistribute it and/or modify

 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version. */

/* FlashSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License
 * along with FlashSim.  If not, see <http://www.gnu.org/licenses/>. */

/****************************************************************************/


#include <cmath>
#include <new>
#include <assert.h>
#include <stdio.h>
#include "ssd.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

using namespace ssd;


RaidSsd::RaidSsd(uint ssd_size):
	size(ssd_size)
{

	Ssds = new Ssd[RAID_NUMBER_OF_PHYSICAL_SSDS];
	one_device_size = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE * BLOCK_SIZE)/2;
	printf("\t ** one device size = %d\n", one_device_size);
return;
}

RaidSsd::~RaidSsd(void)
{
	return;
}

double RaidSsd::event_arrive(enum event_type type, ulong logical_address, uint size, double start_time)
{
	return event_arrive(type, logical_address, size, start_time, NULL);
}

/* This is the function that will be called by DiskSim
 * Provide the event (request) type (see enum in ssd.h),
 * 	logical_address (page number), size of request in pages, and the start
 * 	time (arrive time) of the request
 * The SSD will process the request and return the time taken to process the
 * 	request.  Remember to use the same time units as in the config file. */
double RaidSsd::event_arrive(enum event_type type, ulong logical_address, uint size, double start_time, void *buffer)
{


	if (PARALLELISM_MODE == 1) // Striping
	{
		double timings[RAID_NUMBER_OF_PHYSICAL_SSDS];


		for (int i=0;i<RAID_NUMBER_OF_PHYSICAL_SSDS;i++)
		{
					//printf("\t ** SSD %d event_arrive call\n", i);

						if (buffer == NULL)
						{

							timings[i] = Ssds[i].event_arrive(type, logical_address, size, start_time, NULL);
						}

						else
							timings[i] = Ssds[i].event_arrive(type, logical_address, size, start_time, (char*)buffer +(i*PAGE_SIZE));

			}


			for (int i=0;i<RAID_NUMBER_OF_PHYSICAL_SSDS-1;i++)
			{
				if (timings[i] != timings[i+1])
					fprintf(stderr, "ERROR: Timings are not the same. %d %d\n", timings[i], timings[i+1]);
			}

			return timings[0];
	}

	else if (PARALLELISM_MODE == 2) // Splitted address space
	{
		     int event_received_SSD = logical_address%RAID_NUMBER_OF_PHYSICAL_SSDS;
					ssd::ulong new_array_logical_address = logical_address / RAID_NUMBER_OF_PHYSICAL_SSDS;
					return Ssds[event_received_SSD].event_arrive(type, new_array_logical_address, size, start_time);
	}
}

//send r/w commands to designated SSD
//DN is the SSD# for the event
double RaidSsd::event_arrive_RAID5(enum event_type type, ssd::ulong logical_address, uint size, double start_time,ssd::uint DN)
{
	ssd::ulong new_array_logical_address = logical_address / RAID_NUMBER_OF_PHYSICAL_SSDS;
	long k = Ssds[DN].event_arrive(type, new_array_logical_address, size, start_time);
	return k;
}



double RaidSsd::event_arrive_RAID5_parityWrite(enum event_type type, ssd::ulong logical_address, uint size, double start_time,ssd::uint DN)
{

	ssd::ulong array_logical_address = logical_address %one_device_size;
	long k = Ssds[DN].event_arrive(type, array_logical_address, size, start_time);
	return k;
}

/*
 * Returns a pointer to the global buffer of the Ssd.
 * It is up to the user to not read out of bound and only
 * read the intended size. i.e. the page size.
 */
void *RaidSsd::get_result_buffer()
{
	return global_buffer;
}

void RaidSsd::print_statistics()
{
	for (uint i=0;i<RAID_NUMBER_OF_PHYSICAL_SSDS;i++)
		{
			printf ("%u %s",i, " th SSD \n");
			Ssds[i].print_statistics();
		}


}

void RaidSsd::print_ftl_statistics()
{
	for (uint i=0;i<RAID_NUMBER_OF_PHYSICAL_SSDS;i++)
		{
			printf ("%u %s",i, " th SSD \n");
			Ssds[i].print_ftl_statistics();

		}
}



