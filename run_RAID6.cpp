/* Copyright (c) of KAIST, 2015 */

/*
* SRSim : A simulator for SSD-based RAID (Version 1.0)

* Author : HooYoung Ahn
* Class name : run_RAID6.cpp
* Description : This class distributes r/w evnets to multiple SSDs in RAID 6 architecture
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
    std::map<ssd::ulong, ssd::uint> SN_PN_map;//for first parity
    std::map<ssd::ulong, ssd::uint> SN_PN_1_map;//for second parity
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

    ssd::ulong deviceSize =(SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE * BLOCK_SIZE* RAID_NUMBER_OF_PHYSICAL_SSDS)/2;
     printf("** deviceSize : %d\n", deviceSize);


    // now rewind file and run trace
    fseek(trace, 0, SEEK_SET);


    std::map<ssd::ulong, Stripe*>::iterator stripe_Vaddr_iter ;

    	// step1 : scanning the input trace and arrange pages in RAID 6 format
    while(fgets(line, 80, trace) != NULL){
    	  sscanf(line, "%u,%lu,%u,%c,%lf", &diskno, &vaddr, &size, &op, &arrive_time);
    	  vaddr = vaddr%deviceSize;
    	  op_char = op;


            if(op_char=='w')
            					{

              std::map<ssd::ulong, ssd::uint>::iterator it_DN_map ;
				        //for update data
              it_DN_map = LBN_DN_map.find(vaddr);
            	if(it_DN_map != LBN_DN_map.end())
            							{
            		//printf("\n ===DATA UPDATE===  \n");
            		ssd::uint DN = (*it_DN_map).second;

            		std::map<ssd::ulong, ssd::ulong>::iterator it_SN_map ;
            		it_SN_map = LBN_SN_map.find(vaddr);
            		ssd::uint SN = (*it_SN_map).second;


										write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, DN);

										if (write_time != 0)
										{
										//	tot_dataUpdateCnt++;
											write_total += write_time;
											num_writes++;
										}



										std::map<ssd::ulong, ssd::uint>::iterator it_PN_map ;
										it_PN_map = SN_PN_map.find(SN);
            		//if parity exist, update parity
            		if  (it_PN_map != SN_PN_map.end())
                               		{

            	 ssd::uint PN = (*it_PN_map).second;
              write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, PN);
              if (write_time != 0)
          									{
          										write_total += write_time;
          										num_writes++;
          									}

                               			}


            		//2nd parity update
            		std::map<ssd::ulong, ssd::uint>::iterator it_PN_1_map ;

            		it_PN_1_map = SN_PN_1_map.find(SN);
               if  (it_PN_1_map != SN_PN_1_map.end())
                                           		{
                        	 ssd::uint PN_1 = (*it_PN_1_map).second;
                        	 write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, PN_1);
                        	 if (write_time != 0)
                      									{
                      										write_total += write_time;
                      										num_writes++;
                      									}

                                           			}
            				}

           //for new data write
            	else{

                // 1. find SN  SN=LBN/(N-1)
                ssd::ulong SN = vaddr/(RAID_NUMBER_OF_PHYSICAL_SSDS-2);
                //2. create LBN_SN_map
                LBN_SN_map.insert(std::pair<ssd::ulong,ssd::ulong>(vaddr, SN));

                //2. create stripe and add to SN_stripe map
                stripe_Vaddr_iter = stripe_Vaddr_map.find(SN);
                	if (stripe_Vaddr_iter == stripe_Vaddr_map.end())
                       	    	             {

                		 Stripe *stripe = new Stripe(STRIPE_SIZE);
                   stripe->Vaddrs.push_back(vaddr);
                   stripe->stripeID = SN;
                   stripe_Vaddr_map.insert(std::pair<ssd::ulong, Stripe*>(SN, stripe));
             	    	                  }

                	else if  (stripe_Vaddr_iter != stripe_Vaddr_map.end())
                							{
                		Stripe *stripe = (*stripe_Vaddr_iter).second;
                		stripe->Vaddrs.push_back(vaddr);

                							}


                    //3. find PN     PN=SN mod N
                    ssd::uint PN = SN % RAID_NUMBER_OF_PHYSICAL_SSDS;
                    //4. create SN_PN_map
                    SN_PN_map.insert(std::pair<ssd::ulong,ssd::uint>(SN,PN));

                    ssd::uint PN_1;

                    //3. find PN_1

                    if(PN != RAID_NUMBER_OF_PHYSICAL_SSDS-1)
                    									{
                     	PN_1 = PN+1;
                     	SN_PN_1_map.insert(std::pair<ssd::ulong,ssd::uint>(SN,PN_1));
                    									}
                    else if(PN == RAID_NUMBER_OF_PHYSICAL_SSDS-1)
                    									{
                      	PN_1 = (PN+1) % RAID_NUMBER_OF_PHYSICAL_SSDS;
                     	SN_PN_1_map.insert(std::pair<ssd::ulong,ssd::uint>(SN,PN_1));
                    									}


                    //6.   find DN
                    ssd::uint check = vaddr%(RAID_NUMBER_OF_PHYSICAL_SSDS-2);


                    if(check < PN && check < PN_1)
                               			{

                    	ssd::uint DN = check;
                    	LBN_DN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr,DN));
                               			}

                    else if(PN == check && check <= PN_1)
                    			{
                    	ssd::uint DN = check + 2;
                    	LBN_DN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr,DN));
                    			}

                    else if(PN < check && check > PN_1 )
                              			{
                     ssd::uint DN = check + 2;
                     LBN_DN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr,DN));
                              			}
                    else if(PN < check && check == PN_1)
                                 			{
                      ssd::uint DN = check + 2;
                      LBN_DN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr,DN));
                                 			}
                    else if (PN > check && check >= PN_1)
            					{
                    	ssd::uint DN = check+1;
                    	LBN_DN_map.insert(std::pair<ssd::ulong,ssd::uint>(vaddr,DN));
            					}




									//8. write data to DN
             std::map<ssd::ulong, ssd::uint>::iterator it_LBN_DN_map;
             it_LBN_DN_map = LBN_DN_map.find(vaddr);
									ssd::uint SSDnum = (*it_LBN_DN_map).second;

									write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1, arrive_time, SSDnum);
									if (write_time != 0)
									{
										write_total += write_time;
										num_writes++;
									}



									//9. create parity if stripe->Vaddrs.size() ==  STRIPE_SIZE-1
									 stripe_Vaddr_iter = stripe_Vaddr_map.find(SN);
									if(stripe_Vaddr_iter != stripe_Vaddr_map.end())
									{
										Stripe *stripe = (*stripe_Vaddr_iter).second;
										if (stripe->Vaddrs.size() == RAID_NUMBER_OF_PHYSICAL_SSDS - 2) {

														std::map<ssd::ulong, ssd::uint>::iterator it_SN_PN_map;
														it_SN_PN_map = SN_PN_map.find(SN);
														ssd::uint SSDnum = (*it_SN_PN_map).second;

														write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, SSDnum);
														if (write_time != 0) {
															write_total += write_time;
															num_writes++;
														}

														parityNum ++;

														//for 2nd parity update
											    std::map<ssd::ulong, ssd::uint>::iterator it_SN_PN_1_map;
														it_SN_PN_1_map = SN_PN_1_map.find(SN);
														ssd::uint SSDnum_PN1 = (*it_SN_PN_1_map).second;

														write_time = ssd->event_arrive_RAID5(WRITE, vaddr, 1,arrive_time, SSDnum_PN1);
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

        printf("\n %lu ln", lineNum);
    }


    printf("*************\n");

    printf("Num reads : %lu\n", num_reads);
    printf("Num writes: %lu\n", num_writes);
    printf("Avg read time : %5.20lf\n", read_total / num_reads);
    printf("Avg write time: %5.20lf\n", write_total / num_writes);



    					printf("\n*** Total number of parities  = %lu \n", parityNum);

						   printf("\n*** LBN : SN *** \n");
						   std::map<ssd::ulong, ssd::ulong>::iterator it_LBN_SN_map_iter;
						   for (it_LBN_SN_map_iter = LBN_SN_map.begin(); it_LBN_SN_map_iter != LBN_SN_map.end(); ++it_LBN_SN_map_iter)
						   printf("LBN : SN = %ld, %ld\n ",  (*it_LBN_SN_map_iter).first,  (*it_LBN_SN_map_iter).second);


						   printf("\n *** SN : PN *** ");
						   std::map<ssd::ulong, ssd::uint>::iterator it_SN_PN_map_iter;
						   printf("\n*** Total number of SN = %d \n", SN_PN_map.size());
	  	  	  	 for (it_SN_PN_map_iter = SN_PN_map.begin(); it_SN_PN_map_iter != SN_PN_map.end(); ++it_SN_PN_map_iter)
	          printf("LBN : PN = %ld, %d\n ",  (*it_SN_PN_map_iter).first,  (*it_SN_PN_map_iter).second);



				       printf("\n *** SN : PN_1 *** \n");
				      std::map<ssd::ulong, ssd::uint>::iterator it_SN_PN_1_map_iter;
				      for (it_SN_PN_1_map_iter = SN_PN_1_map.begin(); it_SN_PN_1_map_iter != SN_PN_1_map.end(); ++it_SN_PN_1_map_iter)
					     printf("LBN : PN_1 = %ld, %d\n ",  (*it_SN_PN_1_map_iter).first,  (*it_SN_PN_1_map_iter).second);


	  	  	  	 printf("\n *** LBN : DN *** \n");
	  	  	    std::map<ssd::ulong, ssd::uint>::iterator it_LBN_DN_map_iter;
	  	  	  	 for (it_LBN_DN_map_iter = LBN_DN_map.begin(); it_LBN_DN_map_iter != LBN_DN_map.end(); ++it_LBN_DN_map_iter)
	  	  	  	 printf("LBN : DN = %ld, %d\n",  (*it_LBN_DN_map_iter).first,  (*it_LBN_DN_map_iter).second);




								printf("\n *** SN : stripe *** \n");
								for (stripe_Vaddr_iter = stripe_Vaddr_map.begin();
									stripe_Vaddr_iter != stripe_Vaddr_map.end(); ++stripe_Vaddr_iter) {
									printf("SN = %ld\n", (*stripe_Vaddr_iter).first);
									Stripe *s = (*stripe_Vaddr_iter).second;
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
