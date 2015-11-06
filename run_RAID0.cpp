/* Copyright (c) of KAIST, 2015 */

/*
* SRSim : A simulator for SSD-based RAID (Version 1.0)


* Author : HooYoung Ahn
* Class name : run_RAID0.cpp
* Description : This class distributes r/w evnets to multiple SSDs in RAID 0 architecture
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ssd.h"
 #include <string.h>

using namespace ssd;

int main(int argc, char **argv){
    double arrive_time,base_time;
    ssd::uint diskno;
    ssd::ulong vaddr;
    ssd::uint size;
    ssd::uint op;
    char line[80];
    //double read_time = 0;
    //double write_time = 0;
    double read_total = 0;
    double write_total = 0;
    unsigned long num_reads = 0;
    unsigned long num_writes = 0;
    ssd::ulong lineNum = 0;

    //added to cast int op to char type
    char op_char=' ';

    load_config();
   //load_test_config();
  //  print_config(NULL);
    printf("\n");

    //Ssd ssd;
    //RaidSsd ssd;
		 RaidSsd *ssd = new RaidSsd();

    FILE *trace = NULL;
    if((trace = fopen(argv[1], "r")) == NULL){
        printf("Please provide trace file name\n");
        exit(-1);
    }

    printf("HOO INITIALIZING SSD\n");


    //*0.5 for log space for BAST FTL

   int deviceSize =(SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE * BLOCK_SIZE* RAID_NUMBER_OF_PHYSICAL_SSDS)/2;
   printf("** deviceSize : %d\n", deviceSize);


    //event_arrive(event_type, logical_address, size, start_time)
    /* first go through and write to all read addresses to prepare the SSD */
        //**************************************************
        //spc trace  example
        //0,303567,3584,w,0.000000
        //1st : deviceNo
        //2nd : lba
        //3rd : bytes number which should be divided by 512 to be a block size
        //4th : r/w
        //5th : arrival time which should be amplifiied
        //*************************************************
    printf("\n\n=======> Start Trace\n");

    // now rewind file and run trace
    fseek(trace, 0, SEEK_SET);


    while(fgets(line, 80, trace) != NULL){

    sscanf(line, "%u,%lu,%u,%c,%lf", &diskno, &vaddr, &size, &op, &arrive_time);
    //printf("\n\n===============================Origin addr = %lu\n", vaddr);
    ssd::ulong vaddr_new = vaddr%deviceSize;

        op_char = op;

            if(op_char=='w')
        {

            	double write_time = ssd->event_arrive(WRITE, vaddr_new, 1, arrive_time);


                if(write_time != 0)
                {
                    write_total += write_time;
                    num_writes++;
                }
        }
            //for read
                 else if(op_char == 'r')
                 {

                	 double read_time = ssd->event_arrive(READ, vaddr_new, 1, arrive_time);
                     if(read_time != 0)
                     {
                         read_total += read_time;
                         num_reads++;
                     }

                 }
        else fprintf(stderr, "Bad operation in trace %c\n", op);
            lineNum++;
            printf("\n %lu ", lineNum);
    }
    printf("Num reads : %lu\n", num_reads);
    printf("Num writes: %lu\n", num_writes);

    printf("Total Write time :  %5.20lf\n", write_total);
    printf("Avg read time : %5.20lf\n", read_total / num_reads);
    printf("Avg write time: %5.20lf\n", write_total / num_writes);

    ssd->print_statistics();
    ssd->print_ftl_statistics();


    delete ssd;
    return 0;

}
