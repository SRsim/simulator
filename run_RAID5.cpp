/* Copyright (c) of KAIST, 2015 */

/*
* SRSim : A simulator for SSD-based RAID (Version 1.0)


* Author : HooYoung Ahn
* Class name : run_RAID5.cpp
* Description : This class distributes r/w evnets to multiple SSDs in RAID 5 architecture
* parities on each stripe updated according to updates on stripes
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ssd.h"
 #include <string.h>
#include <vector>
#include <queue>


#include <new>
#include <assert.h>
#include <stdexcept>
#include <algorithm>

using namespace ssd;

int main(int argc, char **argv){
    double arrive_time,base_time;
    ssd::uint diskno;
    ssd::ulong vaddr;
    ssd::uint size;
    ssd::uint op;
    char line[80];
    double read_time = 0;
    double write_time = 0;
    double read_total = 0;
    double write_total = 0;
    unsigned long num_reads = 0;
    unsigned long num_writes = 0;


    //added to cast int op to char type
    char op_char=' ';

    //for large size  SSD
    load_config();

    //for small size SSD
    //load_test_config();
    print_config(NULL);
    printf("\n");

    //Ssd ssd;
    RaidSsd *ssd = new RaidSsd();


    std::map<ssd::ulong, ssd::ulong> LBN_SN_map;
    std::map<ssd::ulong, ssd::uint> SN_PN_map;
    std::map<ssd::ulong, ssd::uint> LBN_DN_map;

    std::map<ssd::ulong, Stripe*> stripe_Vaddr_map;
    ssd::ulong lineNum = 0;
    ssd::ulong parityNum = 0;
    FILE *trace = NULL;
    if((trace = fopen(argv[1], "r")) == NULL){
        printf("Please provide trace file name\n");
        exit(-1);
    }

    printf("INITIALIZING SSD array\n");


     int deviceSize =(SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE * BLOCK_SIZE* RAID_NUMBER_OF_PHYSICAL_SSDS)/2;
     printf("** deviceSize : %d\n", deviceSize);


    // now rewind file and run trace
    fseek(trace, 0, SEEK_SET);


    	// step1 : scanning the input trace and arrange pages in RAID 5 format
    while(fgets(line, 80, trace) != NULL){
    	  sscanf(line, "%u,%lu,%u,%c,%lf", &diskno, &vaddr, &size, &op, &arrive_time);
    	  vaddr = vaddr%deviceSize;

    	  op_char = op;


            if(op_char=='w')
            					{

            	std::map<ssd::ulong, ssd::uint>::iterator it_LBN_DN_map ;
            	it_LBN_DN_map = LBN_DN_map.find(vaddr);
            	if(it_LBN_DN_map != LBN_DN_map.end())
            							{
            		//for updates
            		ssd::uint DN = (*it_LBN_DN_map).second;

            		std::map<ssd::ulong, ssd::ulong>::iterator it_LBN_SN_map ;
            		it_LBN_SN_map = LBN_SN_map.find(vaddr);
            		ssd::ulong SN = (*it_LBN_SN_map).second;


            		//data update
										write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, DN);
										if (write_time != 0)
										{
										//	tot_dataUpdateCnt++;
											write_total += write_time;
											num_writes++;
										}


									std::map<ssd::ulong, ssd::uint>::iterator it_SN_PN_map ;
									it_SN_PN_map = SN_PN_map.find(SN);

            		//If parity exist, update parity
            		if  (it_SN_PN_map != SN_PN_map.end())
                               		{
            	 ssd::uint PN = (*it_SN_PN_map).second;

              write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, PN);
              if (write_time != 0)
          									{
          										write_total += write_time;
          										num_writes++;
          									}

                               			}
            				}

            	//for new data write
            	else{
                // 1. SN=LBN/(N-1)
                ssd::uint SN = vaddr/(RAID_NUMBER_OF_PHYSICAL_SSDS-1);
                //2. create LBN_SN_map
                LBN_SN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr, SN));


                //2. create stripe and add to SN_stripe map
                std::map<ssd::ulong, Stripe*>::iterator stripe_Vaddr_iter_1 ;
                stripe_Vaddr_iter_1 = stripe_Vaddr_map.find(SN);
                	if (stripe_Vaddr_iter_1 == stripe_Vaddr_map.end())
                       	    	             {
                		 Stripe *stripe = new Stripe(STRIPE_SIZE);
                   stripe->Vaddrs.push_back(vaddr);
                   stripe->stripeID = SN;
                   stripe_Vaddr_map.insert(std::pair<ssd::ulong, Stripe*>(SN, stripe));
             	    	                  }

                	else if  (stripe_Vaddr_iter_1 != stripe_Vaddr_map.end())
                							{
                		Stripe *stripe = (*stripe_Vaddr_iter_1).second;
                		stripe->Vaddrs.push_back(vaddr);

                							}


                    //3. find PN    PN=SN mod N
                    ssd::uint PN = SN % RAID_NUMBER_OF_PHYSICAL_SSDS;
                    //4. create SN_PN_map
                    SN_PN_map.insert(std::pair<ssd::ulong,ssd::uint>(SN,PN));


                    //6. find DN. DN= LBN mod (N-1) + 1, PN <= LBN mod (N-1)
                    ssd::uint check = vaddr%(RAID_NUMBER_OF_PHYSICAL_SSDS-1);
                    if(PN<check || PN==check)
                    			{
                    	ssd::uint DN = check + 1;
                    	//7. create LBN_DN_map
                    	LBN_DN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr,DN));
                    			}
                    else
            					{
                    	ssd::uint DN = check;
                    	LBN_DN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr,DN));
            					}


									//8. write data to DN
              std::map<ssd::ulong, ssd::uint>::iterator it_LBN_DN_map_2 ;
              it_LBN_DN_map_2 = LBN_DN_map.find(vaddr);
									ssd::uint SSDnum = (*it_LBN_DN_map_2).second;
									write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1, arrive_time, SSDnum);
									if (write_time != 0)
									{
										write_total += write_time;
										num_writes++;
									}


									//9. if stripe->Vaddrs.size() == STRIPE_SIZE-1, generate parity
									std::map<ssd::ulong, Stripe*>::iterator stripe_Vaddr_iter_2 ;
									stripe_Vaddr_iter_2 = stripe_Vaddr_map.find(SN);
									if(stripe_Vaddr_iter_2 != stripe_Vaddr_map.end())
									{
										Stripe *stripe = (*stripe_Vaddr_iter_2).second;

										if (stripe->Vaddrs.size() == RAID_NUMBER_OF_PHYSICAL_SSDS - 1) {

											std::map<ssd::ulong, ssd::uint>::iterator it_SN_PN_map_2 ;
											it_SN_PN_map_2 = SN_PN_map.find(SN);
														ssd::uint SSDnum = (*it_SN_PN_map_2).second;
														//printf(" Parity SSDnum = %lu\n", SSDnum);
														write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, SSDnum);
														if (write_time != 0) {
															write_total += write_time;
															num_writes++;
														}
														parityNum ++;
										}

									}


            	}


        }


   //for read
        else if(op_char == 'r')
        {

                read_time = ssd->event_arrive(READ, vaddr, 1, arrive_time);
            if(read_time != 0)
            {
                read_total += read_time;
                num_reads++;
            }

        }
        else fprintf(stderr, "Bad operation in trace %c\n", op);

        lineNum++;
//        printf("\n ********************************************* TRACE %lu line *** \n", lineNum);
        printf("\n %lu", lineNum);
    }


    printf("*************\n");

    printf("Num reads : %lu\n", num_reads);
    printf("Num writes: %lu\n", num_writes);
    printf("Avg read time : %5.20lf\n", read_total / num_reads);
    printf("Avg write time: %5.20lf\n", write_total / num_writes);





    					printf("\n*** Total number of parities  = %lu \n", parityNum);

						   printf("\n*** LBN : SN *** \n");
						   std::map<ssd::ulong, ssd::ulong>::iterator LBN_SN_it ;
						   for (LBN_SN_it = LBN_SN_map.begin(); LBN_SN_it != LBN_SN_map.end(); ++LBN_SN_it)
						   printf("LBN : SN = %ld, %ld\n ",  (*LBN_SN_it).first,  (*LBN_SN_it).second);


						   printf("\n *** SN : PN *** ");
						   printf("\n*** Total number of SN = %d \n", SN_PN_map.size());
						   std::map<ssd::ulong, ssd::uint>::iterator SN_PN_it ;
	  	  	  	 for (SN_PN_it = SN_PN_map.begin(); SN_PN_it != SN_PN_map.end(); ++SN_PN_it)
	          printf("LBN : SN = %ld, %d\n ",  (*SN_PN_it).first,  (*SN_PN_it).second);


	  	  	  	 printf("\n *** LBN : DN *** \n");
	  	  	    std::map<ssd::ulong, ssd::uint>::iterator LBN_DN_it ;
	  	  	  	 for (LBN_DN_it = LBN_DN_map.begin(); LBN_DN_it != LBN_DN_map.end(); ++LBN_DN_it)
	  	  	  	 printf("LBN : DN = %ld, %d\n",  (*LBN_DN_it).first,  (*LBN_DN_it).second);




								printf("\n *** SN : stripe *** \n");
								std::map<ssd::ulong, Stripe*>::iterator stripe_Vaddr_iter_3 ;
								for (stripe_Vaddr_iter_3 = stripe_Vaddr_map.begin();
										stripe_Vaddr_iter_3 != stripe_Vaddr_map.end(); ++stripe_Vaddr_iter_3) {
									printf("SN = %ld\n", (*stripe_Vaddr_iter_3).first);
									Stripe *s = (*stripe_Vaddr_iter_3).second;
									int vs = s->Vaddrs.size();
									for (int k = 0; k < vs; k++) {
										printf("Vaddr = %lu\n", s->Vaddrs.at(k));
									}
								}

							ssd->print_statistics();
							ssd->print_ftl_statistics();


							delete ssd;
							return 0;



    return 0;

}
