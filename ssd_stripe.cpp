/* Copyright (c) of KAIST, 2015 */

/*
* SRSim : A simulator for SSD-based RAID (Version 1.0)

* Author : HooYoung Ahn
* Class name : ssd_stripe.cpp
* Description : This class has stripe information of data blocks in the SSD array
* stripe has its size and the statistics shows the data update count and parity update
* count of the stripe
 */

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

using namespace ssd;


Stripe::Stripe(uint stripe_size)
{

	parityExist = false;
	dataUpdateCnt = 0;
	parityUpdateCnt = 0;
	stripeID=0;
	return;
}

Stripe::~Stripe(void)
{
	return;
}

void Stripe::printStripestats()
{

	printf("-----------------\n");
	printf("Stripe Statistics:\n");
	printf("-----------------\n");
	printf("stripe id : %d\n", stripeID);
	printf("stripe dataUpdateCnt : %lu\n", dataUpdateCnt);
	printf("stripe parityUpdateCnt : %lu\n", parityUpdateCnt);
	printf("-----------------\n");
}

