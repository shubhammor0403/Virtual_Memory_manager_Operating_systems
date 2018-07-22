#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#include <sys/mman.h>


#define pagesize 256
#define pageentries 256
#define pagenobits 8
#define framesize 256
#define frameentries 256
#define memsize 65536
#define tlbentries 16


int virtual;
int pageno;
int physical;
int frameno;
int offset;
int value;
int pagetable[pageentries];
int tlb[tlbentries][2];
int tlbfront=-1;
int tlbback=-1;
char memory[memsize];
int memindex=0;


int faultcounter=0;
int tlbcounter=0;
int addresscounter=0;
float faultrate,tlbrate;


int getphysical(int virtual);
int getoffset(int virtual);
int getpageno(int virtual);

void initializept(int n);
void initializetlb(int n);
int consultpt(int pageno);
int consulttlb(int pageno);
void updatetlb(int pageno,int frameno);
int getframe();


int main(int argc,char *argv[])
{
	char *infile;
	char *outfile;
	char *storefile;
	char *storedata;
	int storefd;
	char line[8];
	FILE* inptr;
	FILE* outptr;
	
	
	initializept(-1);
	initializetlb(-1);
	
	
	if(argc!=4)
	{
		printf("enter input and output,and store file names");
		exit(EXIT_FAILURE);
	}
	else
	{
		infile=argv[1];
		outfile=argv[2];
		storefile=argv[3];
		
		
		if((inptr=fopen(infile,"r"))==NULL)
		{
			printf("input file cant be opened \n");
			exit(EXIT_FAILURE);
		}
		
		if((outptr=fopen(outfile,"a"))==NULL)
		{
			printf("output file cant be opened \n");
			exit(EXIT_FAILURE);
		}
		
		storefd=open(storefile,O_RDONLY,0);
		storedata=mmap(NULL,memsize, PROT_READ, MAP_SHARED,storefd,0);
		if(storedata==MAP_FAILED)
		{
			close(storefd);
			printf("error mapping the backing store file");
			exit(EXIT_FAILURE);		
		}
		
		while(fgets(line,sizeof(line),inptr))
		{
		 	virtual=atoi(line);
		 	addresscounter++;
			
			
			pageno=getpageno(virtual);
			offset=getoffset(virtual);
			
			
			frameno=consulttlb(pageno);
			if(frameno!=-1)
			{	
				physical=frameno+offset;
				value=memory[physical];
				printf("(TLB) ");
			}
			else
			{
				frameno=consultpt(pageno);
				if(frameno!=-1)
				{
					physical=frameno+offset;
					updatetlb(pageno,frameno);
					value=memory[physical];
					printf("(PT) ");
				}
				
				
				else
				{
					int pageaddress=pageno*pagesize;
					if(memindex!=-1)
					{
						memcpy(memory+memindex,storedata+pageaddress,pagesize);
						frameno=memindex;
						physical=frameno+offset;
						value=memory[physical];
						
						
						pagetable[pageno]=memindex;
						updatetlb(pageno,frameno);
						if(memindex<memsize-framesize)
						{
							memindex+=framesize;
							
						}
						else
						{
						 	memindex=-1;
						 	
						}
						printf("(PF) ");
					}
					
					else
					{
					}
					
				}
			}
			
			fprintf(outptr,"virtual address: %d\t",virtual);
			fprintf(outptr,"Physical address: %d\t",physical);
			fprintf(outptr,"value: %d\n",value);
			printf("virtual address: %d\t",virtual);
			printf("Physical address: %d\t",physical);
			printf("value: %d\n",value);
		}
		faultrate=((float)faultcounter/(float)addresscounter)*100;
		tlbrate=((float)tlbcounter/(float)addresscounter)*100;
		
		fprintf(outptr,"Number of translated addreses = %d\n",addresscounter);
		fprintf(outptr,"Number of page faults = %d pagefaultrate ==%.1f percent	\n",faultcounter,faultrate);
		fprintf(outptr,"Number of tlb hits = %d TLB hit rate ==%.1f percent\n",tlbcounter,tlbrate);
		
		printf("Number of translated addreses = %d\n",addresscounter);
		printf("Number of page faults = %d  pagefaultrate ==%.1f percent	\n",faultcounter,faultrate);
		printf("Number of tlb hits = %d  TLB hit rate ==%.1f percent\n",tlbcounter,tlbrate);
		
		fclose(inptr);
		fclose(outptr);
		close(storefd);
	}
	return EXIT_SUCCESS;
}

int getphysical(int virtual)
{
	physical= getpageno(virtual)+getoffset(virtual);
	return physical;
}

int getpageno(int virtual){
	return(virtual>>pagenobits);
}

int getoffset(int virtual){
	int mask = 255;
	return virtual & mask;
	
}

void initializept(int n)
{
	for(int i=0;i<pageentries;i++){
		pagetable[i]=n;
	}
}

void initializetlb(int n)
{
	for(int i=0;i<tlbentries;i++){
		tlb[i][0]=n;
		tlb[i][1]=n;
	}
}

int consultpt(int pageno){
	if(pagetable[pageno]==-1){
		faultcounter++;
	}
	return pagetable[pageno];
}

int consulttlb(int pageno){
	for(int i=0;i<tlbentries;i++){
		if(tlb[i][0]==pageno){
			tlbcounter++;
			return tlb[i][1];
		}
	}
	return -1;
}

void updatetlb(int pageno,int frameno){
	if(tlbfront==-1){
		tlbfront=0;
		tlbback=0;
		tlb[tlbback][0]=pageno;
		tlb[tlbback][1]=frameno;
		
	}
	else{
		tlbfront=(tlbfront+1)%tlbentries;
		tlbback=(tlbback+1)%tlbentries;
		
		tlb[tlbback][0]=pageno;
		tlb[tlbback][1]=frameno;
	}
	return;
}
