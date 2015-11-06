/* Copyright 2011 Matias Bjørling */

/* Block Management

 *
 * This class handle allocation of block pools for the FTL
 * algorithms.
 */

/* Copyright (c) of KAIST, 2015 */

/*
* SRSim : A simulator for SSD-based RAID (Version 1.0)

* Author : HooYoung Ahn
* Class name : ssd_bm.cpp
* Description : This class has stripe information of data blocks in the SSD array
* stripe has its size and the statistics shows the data update count and parity update
* count of the stripe
 */


#include <new>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <queue>
#include "ssd.h"

using namespace ssd;


Block_manager::Block_manager(FtlParent *ftl) : ftl(ftl)
{
	/*
	 * Configuration of blocks.
	 * User-space is the number of blocks minus the
	 * requirements for map directory.
	 */

	//printf("\n==Block_manager created== NUMBER_OF_ADDRESSABLE_BLOCKS = %d\n", NUMBER_OF_ADDRESSABLE_BLOCKS);

	max_blocks = NUMBER_OF_ADDRESSABLE_BLOCKS;
	max_log_blocks = max_blocks;

	if (FTL_IMPLEMENTATION == IMPL_FAST)
		max_log_blocks = FAST_LOG_PAGE_LIMIT;

	// Block-based map lookup simulation
	max_map_pages = MAP_DIRECTORY_SIZE * BLOCK_SIZE;

	directoryCurrentPage = 0;
	num_insert_events = 0;

	data_active = 0;
	log_active = 0;

	current_writing_block = -2;

	out_of_blocks = false;

	active_cost.reserve(NUMBER_OF_ADDRESSABLE_BLOCKS);

	simpleCurrentFree = 0;
}

Block_manager::~Block_manager(void)
{
	return;
}

void Block_manager::cost_insert(Block *b)
{
	active_cost.push_back(b);
}

void Block_manager::instance_initialize(FtlParent *ftl)
{
	Block_manager::inst = new Block_manager(ftl);
}
/*

Block_manager *Block_manager::instance()
{
	return Block_manager::inst;
}
*/

Block_manager *Block_manager::instance()
{
	return this;
}

/*
 * Retrieves a page using either simple approach (when not all
 * pages have been written or the complex that retrieves
 * it from a free page list.
 */
void Block_manager::get_page_block(Address &address, Event &event)
{
	//printf("\n ===Block_manager::get_page_block===\n");
	//printf("\n ===simpleCurrentFree = %lu \n", simpleCurrentFree);
	//printf("\n ===BLOCK_SIZE = %d \n", BLOCK_SIZE);

	//printf("\n ===max_blocks = %d \n", max_blocks);
	//printf("\n ===max_blocks*BLOCK_SIZE = %lu \n", max_blocks*BLOCK_SIZE);
	// We need separate queues for each plane? communication channel? communication channel is at the per die level at the moment. i.e. each LUN is a die.
	if (simpleCurrentFree < max_blocks*BLOCK_SIZE)
	{
		address.set_linear_address(simpleCurrentFree, BLOCK);
		current_writing_block = simpleCurrentFree;
		simpleCurrentFree += BLOCK_SIZE;
	}
	else
	{

		if (free_list.size() <= 1 && !out_of_blocks)
		{
			out_of_blocks = true;
			insert_events(event);
		}
		assert(free_list.size() != 0);
		address.set_linear_address(free_list.front()->get_physical_address(), BLOCK);
		current_writing_block = free_list.front()->get_physical_address();
		free_list.erase(free_list.begin());
		out_of_blocks = false;
	}
}


Address Block_manager::get_free_block(Event &event)
{
	return get_free_block(DATA, event);
}

/*
 * Handles block manager statistics when changing a
 * block to a data block from a log block or vice versa.
 */
void Block_manager::promote_block(block_type to_type)
{
	if (to_type == DATA)
	{
		data_active++;
		log_active--;
	}
	else if (to_type == LOG)
	{
		log_active++;
		data_active--;
	}
}

/*
 * Returns true if there are no space left for additional log pages.
 */
bool Block_manager::is_log_full()
{
	return log_active == max_log_blocks;
}

void Block_manager::print_statistics()
{
	printf("-----------------\n");
	printf("Block Statistics:\n");
	printf("-----------------\n");
	printf("Log blocks:  %lu\n", log_active);
	printf("Data blocks: %lu\n", data_active);
	printf("Free blocks: %lu\n", (max_blocks - (simpleCurrentFree/BLOCK_SIZE)) + free_list.size());
	printf("Invalid blocks: %lu\n", invalid_list.size());
	printf("Free2 blocks: %lu\n", (unsigned long int)invalid_list.size() + (unsigned long int)log_active + (unsigned long int)data_active - (unsigned long int)free_list.size());
	printf("-----------------\n");


}

void Block_manager::invalidate(Address address, block_type type)
{
	invalid_list.push_back(ftl->get_block_pointer(address));

	switch (type)
	{
	case DATA:
		data_active--;
		break;
	case LOG:
		log_active--;
		break;
	case LOG_SEQ:
		break;
	}
}

/*
 * Insert erase events into the event stream.
 * The strategy is to clean up all invalid pages instantly.
 */
void Block_manager::insert_events(Event &event)
{
	//printf("\n\tBlock_manager::insert_events +++++++++++++++\n");
//printf("\n invalid_list.size () = %d \n", invalid_list.size());
// Calculate if GC should be activated.
float used = (int)invalid_list.size() + (int)log_active + (int)data_active - (int)free_list.size();
float total = NUMBER_OF_ADDRESSABLE_BLOCKS;
float ratio = used/total;

if (ratio < 0.90) // Magic number
	return;

uint num_to_erase = 5; // More Magic!
//printf("\nBlock_manager::insert_events num_to_erase = %d %i %i %i\n", num_to_erase, invalid_list.size(), log_active, data_active);


	// First step and least expensive is to go though invalid list. (Only used by FAST)
while (num_to_erase != 0 && invalid_list.size() != 0)
	{
		Event erase_event = Event(ERASE, event.get_logical_address(), 1, event.get_start_time());
		erase_event.set_address(Address(invalid_list.back()->get_physical_address(), BLOCK));
		if (ftl->controller.issue(erase_event) == FAILURE) {	assert(false);}
		event.incr_time_taken(erase_event.get_time_taken());

		free_list.push_back(invalid_list.back());
		invalid_list.pop_back();

		num_to_erase--;
		ftl->controller.stats.numFTLErase++;
		//printf("\nBlock_manager::insert_events ftl->controller.stats.numFTLErase++ \n");

	}

	num_insert_events++;

	if (FTL_IMPLEMENTATION == IMPL_DFTL || FTL_IMPLEMENTATION == IMPL_BIMODAL)
	{

		ActiveByCost::iterator it = active_cost.get<1>().end();
		--it;


		while (num_to_erase != 0 && (*it)->get_pages_invalid() > 0 && (*it)->get_pages_valid() == BLOCK_SIZE)
		{
			if (current_writing_block != (*it)->physical_address)
			{
				//printf("erase p: %p phy: %li ratio: %i num: %i\n", (*it), (*it)->physical_address, (*it)->get_pages_invalid(), num_to_erase);
				Block *blockErase = (*it);

				// Let the FTL handle cleanup of the block.
				ftl->cleanup_block(event, blockErase);

				// Create erase event and attach to current event queue.
				Event erase_event = Event(ERASE, event.get_logical_address(), 1, event.get_start_time());
				erase_event.set_address(Address(blockErase->get_physical_address(), BLOCK));

				// Execute erase
				if (ftl->controller.issue(erase_event) == FAILURE) { assert(false);	}

				free_list.push_back(blockErase);

				event.incr_time_taken(erase_event.get_time_taken());

				ftl->controller.stats.numFTLErase++;
			}

			it = active_cost.get<1>().end();
			--it;

			if (current_writing_block == (*it)->physical_address)
			 --it;

			num_to_erase--;
		}
	}
}

Address Block_manager::get_free_block(block_type type, Event &event)
{
	//printf("Block_manager::get_free_block+++++ ");
	Address address;
	get_page_block(address, event);
	switch (type)
	{
	case DATA:

		ftl->controller.get_block_pointer(address)->set_block_type(DATA);
		data_active++;
		break;
	case LOG:
		//printf("max_log_blocks = %lu\n", max_log_blocks*RAID_NUMBER_OF_PHYSICAL_SSDS);
		//printf("log_active = %lu\n", log_active);

		if (log_active > max_log_blocks)
		//if (log_active > max_log_blocks*RAID_NUMBER_OF_PHYSICAL_SSDS)
			throw std::bad_alloc();

		ftl->controller.get_block_pointer(address)->set_block_type(LOG);
		log_active++;
		break;
	default:
		break;
	}
	return address;
}

void Block_manager::print_cost_status()
{

	ActiveByCost::iterator it = active_cost.get<1>().begin();

	for (uint i=0;i<10;i++) //SSD_SIZE*PACKAGE_SIZE*DIE_SIZE*PLANE_SIZE
	{
		printf("%li %i %i\n", (*it)->physical_address, (*it)->get_pages_valid(), (*it)->get_pages_invalid());
		++it;
	}

	printf("end:::\n");

	it = active_cost.get<1>().end();
	--it;

	for (uint i=0;i<10;i++) //SSD_SIZE*PACKAGE_SIZE*DIE_SIZE*PLANE_SIZE
	{
		printf("%li %i %i\n", (*it)->physical_address, (*it)->get_pages_valid(), (*it)->get_pages_invalid());
		--it;
	}
}

void Block_manager::erase_and_invalidate(Event &event, Address &address, block_type btype)
{
	//printf("\n \t Block_manager::erase_and_invalidate event.get_start_time() %5.20lf \n",event.get_start_time());

	Event erase_event = Event(ERASE, event.get_logical_address(), 1, event.get_start_time()+event.get_time_taken());

	erase_event.set_address(address);

	if (ftl->controller.issue(erase_event) == FAILURE) { assert(false);}

	//printf("\n \t Block_manager +++++++++++++++ =%5.20lf\n",erase_event.get_time_taken());

	free_list.push_back(ftl->get_block_pointer(address));

			switch (btype)
			{
			case DATA:
				data_active--;
				break;
			case LOG:
				log_active--;
				break;
			case LOG_SEQ:
				break;
			}


	event.incr_time_taken(erase_event.get_time_taken());
	ftl->controller.stats.numFTLErase++;
}

int Block_manager::get_num_free_blocks()
{
	if (simpleCurrentFree < max_blocks*BLOCK_SIZE)
		return (simpleCurrentFree / BLOCK_SIZE) + free_list.size();
	else
		return free_list.size();
}

void Block_manager::update_block(Block * b)
{
	std::size_t pos = (b->physical_address / BLOCK_SIZE);
	active_cost.replace(active_cost.begin()+pos, b);
}
