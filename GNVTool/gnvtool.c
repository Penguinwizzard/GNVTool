#define __extension__ 
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#ifndef __cplusplus
#define true 1
#define false 0
typedef unsigned char bool;
#endif

char* helpmsg = "GNVTool 1.5, by Penguinwizzard\nusage: GNVTool <option> <inputfile> [<outputfile>] [<offsetx> <offsety>]\nOptions:\ninfo\tDisplays info about the gnv file\ntopbm\tconverts gnv to pgm\ntognv\tconverts pgm to gnv\n";
typedef struct GNVheader_t
{
	unsigned int magic;
	float gridsize;
	float xoffset;
	float yoffset;
	unsigned int xstep;
	unsigned int ystep;
	int xalign;
	int yalign;
	char entries[];
} GNVheader;

char lookup[16] = {
   0x0, 0x8, 0x4, 0xC,
   0x2, 0xA, 0x6, 0xE,
   0x1, 0x9, 0x5, 0xD,
   0x3, 0xB, 0x7, 0xF };
char flip( char n )
{
   return (lookup[n&0x0F] << 4) | lookup[n>>4];
}

void readpbm(const char* infile,unsigned char** buf,unsigned int* width, unsigned int* height);

unsigned long load_file(unsigned char** buf, const char* fname, const char* opts, const char* errmsg) {
	FILE* file;
	unsigned long fsize;
	fopen_s(&file,fname,opts);
	if(file == NULL) {
		printf("%s",errmsg);
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	fsize = (unsigned long)ftell(file);
	fseek(file, 0, SEEK_SET);

	(*buf) = (unsigned char*)malloc((size_t)fsize);
	fread(*buf, fsize, 1, file);
	fclose(file);
	return fsize;
}

int main(int argc, char* argv[]) {
	if(argc <3) {
		printf(helpmsg);
		return 0;
	}
	if(!strncmp(argv[1],"info",4)) {
		unsigned long fsize;
		unsigned long bodysize;
		GNVheader* header;

		fsize = load_file((unsigned char**)&header,argv[2],"rb","Error: Could not load GNV file\n");

		bodysize = fsize-32;
		printf("File loaded, general file info:\n");
		printf("Sanity Check: %.8X ",header->magic);
		if(header->magic==0xFADEBEAD) {
			printf("(passed)\n");
		} else {
			printf("(failed)\n");
		}
		printf("Grid Edge Length: %f\n",header->gridsize);
		printf("X Grid offset: %f\n",header->xoffset);
		printf("Y Grid offset: %f\n",header->yoffset);
		printf("XSteps: %u\n",header->xstep);
		printf("YSteps: %u\n",header->ystep);
		printf("X Alignment: %i\n",header->xalign);
		printf("Y Alignment: %i\n",header->yalign);
		printf("Body Size: %lu\n",bodysize);
		free(header);
	} else if(!strncmp(argv[1],"topbm",5)) {
		unsigned long fsize;
		GNVheader *header;
		unsigned long bodysize;
		FILE* target;
		int row;
		unsigned int col;
		errno_t err;

		fsize = load_file((unsigned char**)&header,argv[2],"rb","Error: Could not load GNV file\n");
		bodysize = fsize-32;
		if(argc < 4) {
			printf("Specify output file\n");
			return 0;
		}
		err = fopen_s(&target,argv[3],"wb");
		if(err != 0) {
			printf("Could not open pbm file for writing.\n");
			exit(1);
		}
		fprintf(target,"P1\n%i %i\n",header->xstep,header->ystep);
		for(row=header->ystep-1;row>=0;row--) {
			for(col=0;col<header->xstep;col++) {
				if(header->entries[row*(header->xstep)+(col)]==1) {
					fprintf(target,"0 ");
				} else {
					fprintf(target,"1 ");
				}
			}
			fprintf(target,"\n");
		}
		fclose(target);
		free(header);
	} else if(!strncmp(argv[1],"tognv",5)) {
		unsigned char *buf;
		unsigned char *inbuf;
		FILE *gnvfile;
		GNVheader *header;
		unsigned int bodysize;
		unsigned int row,column;
		unsigned int height,width;
		errno_t err;

		if(argc < 4) {
			printf("Specify output file\n");
			return 0;
		}
		if(argc < 6) {
			printf("Specify offset values\n");
			return 0;
		}

		readpbm(argv[2],&inbuf,&width,&height);

		bodysize = 32 + height*width;

		buf = (unsigned char*)malloc(bodysize);
		memset(buf,0,bodysize);
		header = (GNVheader*)buf;
		header->magic = 0xFADEBEAD;
		header->gridsize = 64.0f;
		header->xoffset = 32.0f;
		header->yoffset = 32.0f;
		header->xstep = width;
		header->ystep = height;
		header->xalign = atoi(argv[4]);
		header->yalign = atoi(argv[5]);

		for(row = 0; row < height; row++) {
			for(column = 0; column < width; column++) {
				if(!(inbuf[((height-row-1)*width+column)/8] & (1 << (7-(column%8))))) {
					header->entries[((row)*header->xstep+column)] = 1;
				}
			}
		}
		err = fopen_s(&gnvfile,argv[3],"wb");
		if(err != 0) {
			printf("Could not open GNV file for writing.\n");
			exit(1);
		}
		fwrite(buf,bodysize,1,gnvfile);
		fclose(gnvfile);
	} else if(!strncmp(argv[1],"update",6)) {
		long fsize;
		GNVheader *header;
		FILE* target;
		unsigned char *buf2;
		GNVheader *newheader;
		unsigned int i;
		errno_t err;

		fsize = load_file((unsigned char**)&header,argv[2],"rb","Error: Could not load GNV file\n");
		if(fsize-32 == header->xstep*header->ystep) {
			printf("GNV File is already updated, exiting...\n");
			exit(0);
		}

		buf2 = (unsigned char*)malloc(32+header->xstep*header->ystep);
		newheader = (GNVheader*)buf2;
		memcpy(buf2,header,32);//copy the header info

		
		for(i=0;i<header->xstep*header->ystep;i++) {
			newheader->entries[i]=(header->entries[i/8] & (1<<(i%8)))==0?0:1;
		}
		err = fopen_s(&target,argv[2],"wb");
		if(err != 0) {
			printf("Could not re-open GNV file for writing.\n");
			exit(1);
		}
		fwrite(buf2,32+header->xstep*header->ystep,1,target);
		fclose(target);
		free(header);
		free(buf2);
	} else {
		printf(helpmsg);
	}
	return 0;
}
//Skip whitespace
unsigned char nextvalid(unsigned char* buf, int* index) {
	while(buf[*index]==' '||buf[*index]=='\n'||buf[*index]=='\t'||buf[*index]=='\r') {
		(*index)++;
	}
	return buf[(*index)];
}
//Skip the rest of the current line
void skipline(unsigned char* buf, int* index) {
	while(buf[*index]!='\n' && buf[*index]!='\r') {
		(*index)++;
	}
	(*index)++;
}
//Skip non-whitespace
unsigned char nextinvalid(unsigned char* buf, int* index) {
	while(buf[*index]!=' '&&buf[*index]!='\n'&&buf[*index]!='\t' && buf[*index]!='\r') {
		(*index)++;
	}
	return buf[*index];
}
void readpbm(const char* infile,unsigned char** buf,unsigned int* width, unsigned int* height) {
	bool binary;
	int index;
	unsigned char* inbuf;
	long fsize;
	char cur;

	fsize = load_file(&inbuf,infile,"rb","Could not load pbm file.\n");

	index=0;
	cur = nextvalid(inbuf,&index);
	index++;
	if(cur != 'P') {
		printf("ERROR 1: INVALID PBM FILE HEADER%c\n",cur);
		exit(0);
	}
	cur = nextvalid(inbuf,&index);
	index++;
	if(cur == '1') {
		binary=false;
	} else if(cur=='4') {
		binary=true;
	} else {
		printf("ERROR 2: INVALID PBM FILE HEADER\nCheck that your file is a P1 or P4 (PBM) file.\n");
		exit(0);
	}
	cur=nextvalid(inbuf,&index);
	while(cur=='#') {
		skipline(inbuf,&index);
		cur=nextvalid(inbuf,&index);
	}
	*width = atoi((const char*)&inbuf[index]);
	cur=nextinvalid(inbuf,&index);
	cur=nextvalid(inbuf,&index);
	while(cur=='#') {
		skipline(inbuf,&index);
		cur=nextvalid(inbuf,&index);
	}
	*height = atoi((const char*)&inbuf[index]);
	*buf=(unsigned char*)malloc((*width)*(*height)/8);
	cur=nextinvalid(inbuf,&index);//one whitespace between height and data
	cur=nextvalid(inbuf,&index);
	if(binary) {
		memcpy(*buf,&inbuf[index],(*width)*(*height)/8);
	} else {
		unsigned int bitindex=0;
		memset(*buf,0,(*width)*(*height)/8);
		while(bitindex < (*width)*(*height)) {
			cur=nextvalid(inbuf,&index);
			if(cur!='0') {
				(*buf)[bitindex/8] |= (1<<(7-(bitindex%8)));
			}
			bitindex++;
			index++;
		}
	}
	return;
}