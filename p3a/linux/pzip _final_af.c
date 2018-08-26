#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <fcntl.h>

// constants
#define BUFFER (32)
int qsize = 32;

//struct definitions
typedef struct {
	char *start;
	size_t index;
	size_t size;
} arg_struct;

typedef struct {
	char *final;
	size_t size;
} stitched;

//global variables
int pgsize;						//page size
arg_struct *queue;				//producer/consumer queue
static int fillptr = 0;
static int useptr = 0;
static int numfull = 0;
stitched *lilfinal;				//array to hold compressed pairs
static int chunks;				//number of chunks to be processed
volatile int living;			//terminating condition
//lock stuff
pthread_mutex_t m  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;


//method declarations
void *consumer(void *ptr);
void do_fill(arg_struct chunk);
arg_struct do_get();
void rle(arg_struct chunk, stitched *tmp);
void *consumer(void *ptr);

int main(int argc, char **argv)
{
	if (argc <= 1) 
	{
		printf("pzip: file1 [file2 ...]\n");
		exit(1);
	}
	living = 1;
	pgsize = getpagesize()*8;
	int nprocs = get_nprocs();
	int numfiles = argc - 1;
	int lf_cnt = 0;
	chunks = 0;
	int fil = 0;
	int next = 1;
	int sizeinfileleft = 0;
	int offset = 0;
	int chunksize = 0;
	void *map = NULL;

	//calculate total number of chunks to be processed
	for (int i = 1; i < numfiles + 1; i++) 
	{
		int fd = open(argv[i], O_RDONLY);

		struct stat statbuf;
		fstat(fd, &statbuf);
		double fsize = (double) statbuf.st_size;

		int a = fsize / ((double)pgsize);
		if ( ((size_t) fsize) % pgsize != 0)
		a++;
		if (fsize != 0)
		chunks += a;
		close(fd);
	}

	//allocate
	queue = malloc(sizeof(arg_struct) * qsize);
	lilfinal = malloc(sizeof(stitched) * chunks);

	//create threads
	pthread_t thrds[nprocs];
	for (int i = 0; i < nprocs; i++) 
	{
		pthread_create(&thrds[i], NULL, consumer, NULL);
	}

	//loop over files and prepare chunks to be added to queue
	for (int file = 1; file < numfiles + 1;) 
	{
		if (next) 
		{
			fil = open(argv[file], O_RDONLY);

			struct stat statbuf;
			fstat(fil, &statbuf);
			sizeinfileleft = (size_t) statbuf.st_size;
			offset = 0;
			next = 0;
		}

		chunksize = sizeinfileleft > pgsize ? pgsize : sizeinfileleft;

		if (chunksize == 0) 
		{
			file++;
			close(fil);
			next = 1;
			continue;
		}

		//map chunks
		map = mmap(NULL, chunksize, PROT_READ , MAP_PRIVATE, fil, offset);

		arg_struct args;
		args.start = map;
		args.size  = chunksize;
		args.index = lf_cnt;

		//producer
		pthread_mutex_lock(&m);
		while (numfull == qsize)
		pthread_cond_wait(&empty, &m);
		do_fill(args);
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);

		sizeinfileleft -= chunksize;
		offset  += chunksize;

		if (sizeinfileleft <= 0) 
		{
			file++;
			close(fil);
			next = 1;
		}

		lf_cnt++;
	}
	living = 0;
	
	pthread_cond_broadcast(&fill);
	for (int i = 0; i < nprocs; i++) 
	{
		pthread_join(thrds[i], NULL);
	}

	// Done with threading
	//Final stitch
	//printf("\ndone\n");

	char *end = NULL;

	//Iterate over files and if adjacent letters are the same then add up counts.
	//Print out in binary
	for (int i = 0; i < chunks; i++) 
	{
		char *bin = lilfinal[i].final;
		int n = lilfinal[i].size;

		if (end && end[4] == bin[4]) 
		{
			*((int*)bin) += *((int*)end);
		} else if(end) 
		{
			fwrite(end, 5, 1, stdout);
		}
		fwrite(bin, n - 5, 1, stdout);
		end = bin + n - 5;
	}

	fwrite(end, 5, 1, stdout);

	return 0;
}

void do_fill(arg_struct chunk)
{
	queue[fillptr] = chunk;
	fillptr = (fillptr + 1) % qsize;
	numfull++;
}

arg_struct do_get()
{
	arg_struct ret = queue[useptr];
	useptr = (useptr + 1) % qsize;
	numfull--;
	return ret;
}

void *consumer(void *ptr) 
{
	while (1) 
	{
		arg_struct job;
		pthread_mutex_lock(&m);
		while (numfull == 0 && living)
		{
			pthread_cond_wait(&fill, &m);
		}
		if (numfull == 0 && !living)
		{
			pthread_mutex_unlock(&m);
			pthread_exit(0);
		}
		job = do_get();
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		stitched *tmp = &lilfinal[job.index];
		rle(job, tmp);
	}
	pthread_exit(0);
}

//function to do run length encoding
void rle(arg_struct chunk, stitched *tmp)
{
	char* start = chunk.start;
	char end = *start;
	char *output = malloc(chunk.size * 8);
	char *curr = output;
	int count = 0;
	size_t len = chunk.size;
	// for(int i = 0; i< numelm; i++) {
	// 	printf("Pair=(%d%c) index=(%d)\n",lilfinal[args->index].count[i],lilfinal[args->index].letter[i],args->index);
	// }
	for (int i = 0; i < len; ++i)
	{
		if(start[i] != end)
		{
			*((int*)curr) = count;
			curr[4] = end;
			curr += 5;
			end = start[i];
			count = 1;
		} else count++;
	}

	*((int*)curr) = count;
	curr[4] = start[len-1];
	curr += 5;
	size_t fin_size = curr-output;

	//		printf("INDEX=(%d) cnt: (%d) char: (%c) numelm: (%d)\n", args->index,count, stitched[i],numelm);
	char *res = malloc(fin_size);
	memcpy(res, output, curr-output);
	tmp->size = fin_size;
	tmp->final = res;

	free(output);
	munmap(chunk.start, chunk.size);
	return;
}
