#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysinfo.h> //nprocs
#include <pthread.h>  //pthread

// constants
#define BUFFER (32)

// struct containing information for the run length encoder
typedef struct
{
	char *start;
	int size;
	int index;
} arg_struct;

typedef struct {
	char* letter;
	int* count;
	int numelm;
} stitched;
// typedef struct
// {
// 	char *letter;
// 	int *count;
// 	int numelm;
// } stitched;

// global variables
arg_struct **queue; // producer adds jobs to queue, consumer pulls jobs
int fillptr;  //
int useptr;   //
int numfull;  //
int max;   //
char *map;   //
int nprocs;
int chunks;
int pgsize;
int files;
//char * tmp;
int living;
// method declarations
void *rle(arg_struct* args);
void *producer(void *arguments);
void *consumer(void *arg);
void do_fill(arg_struct *arguments);
arg_struct *do_get();

// initialize mutex locks and conditions
// i removed the second lock
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
stitched* lilfinal;
void do_fill(arg_struct *arguments)
{
	queue[fillptr] = arguments;
	fillptr = (fillptr + 1) % max;
	numfull++;
	chunks++;
}

arg_struct *do_get()
{
	arg_struct *tmp = queue[useptr];
	useptr = (useptr + 1) % max;
	numfull--;
	chunks--;
	return tmp;
}

void *consumer(void *arg)
{
	//printf("numfull#=(%d), and chunks#=(%d)\n",numfull,chunks);
	while (1)
	{
		pthread_mutex_lock(&m);
		while (numfull == 0 && living)
			pthread_cond_wait(&fill, &m);
		if(numfull == 0 && !living) {
			pthread_mutex_unlock(&m);
			pthread_exit(0);
		}
		printf("files#=(%d), and chunks#=(%d)\n",files,chunks);
		arg_struct *con_arg = do_get();
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		rle(con_arg);
	}
}

void *rle(arg_struct* args)
{
	//	printf("tid: %lu\n", tid);

	//int len = (int)(args->end - args->start) + 1;
	int len = args->size;
	//if(args->eof) {
	//	terminate_all = 1;
	//}
	char *output = (char *)calloc(len, sizeof(char));
	strncpy(output, args->start, len);
	//char *output = (char*) malloc(BUFFER* sizeof(char));

	stitched s;
	s.count = malloc(sizeof(int));
	s.letter = malloc(sizeof(char));
	int numelm = 0;
	for(int i = 0 ; i < len ; i++) {
		int count = 1;
		while( i +1 < len && output[i] == output[i+1]) {
			++count;
			++i;
		}
//		printf("INDEX=(%d) cnt: (%d) char: (%c) numelm: (%d)\n", args->index,count, output[i],numelm);
		s.count = realloc(s.count, sizeof(int)*(i+1));
		s.letter = realloc(s.letter, sizeof(char)*(i+1));
		(s.count[numelm]) = count;
		(s.letter[numelm]) = output[i];
		(s.numelm) = numelm++;
	}

	lilfinal[args->index] = s;
	// for(int i = 0; i< numelm; i++) {
	// 	printf("Pair=(%d%c) Idx=(%d)\n",lilfinal[args->index].count[i],lilfinal[args->index].letter[i],args->index);
	// }
	int rc = munmap(args->start, pgsize);
	if (rc == -1) {
		printf("pzip: cannot unmap chunk\n"); //file could not be mapped
		exit(1);
	}

	free(output);
	output = NULL;
	return NULL;
}

int main(int argc, char *argv[])
{
	living = 1;
	if (argc <= 1)
	{
		printf("my-zip: file1 [file2 ...]\n");
		exit(1);
	}

	files = argc-1;
	nprocs = get_nprocs();
	pgsize = getpagesize();
	int lf_cnt = 0;
	numfull = 0;
	pthread_t thrds[nprocs];
	for (int npr = 0; npr < nprocs; npr++)
	{
		pthread_create(&thrds[npr], NULL, consumer, NULL);
	}

	lilfinal = malloc(sizeof(stitched));

	arg_struct *args = NULL;
	queue = (arg_struct **)malloc(max * sizeof(arg_struct *));

	int fd;
	for (int file = 1; file < argc; file++)
	{
		fd = open(argv[file], O_RDONLY);
		if (fd == -1)
		{
			printf("pzip: cannot open file\n"); //file could not be opened
			exit(1);
		}



		struct stat statbuf;
		fstat(fd, &statbuf);
		int fsize = statbuf.st_size;

		max = fsize / pgsize == 0 ? 1 : statbuf.st_size / pgsize;
		printf("max: (%d)\n",max);
		queue = (arg_struct **)realloc(queue, max * sizeof(arg_struct *));
		// iterate through the number of chunks
		int sizeinfileleft = fsize;
		for (int i = 0; i < max; i++) {

			int chunksize = 0;
			if(sizeinfileleft >= pgsize) {
				map = mmap(0, pgsize, PROT_READ, MAP_PRIVATE, fd, i * pgsize);
				chunksize = pgsize;
			}
			else {
				map = mmap(0, sizeinfileleft, PROT_READ, MAP_PRIVATE, fd, i * pgsize);
				chunksize = sizeinfileleft;
			}
			sizeinfileleft -= pgsize;
			if (map == MAP_FAILED)
			{
				printf("pzip:b cannot map chunk\n"); //file could not be mapped
				exit(1);
			}


			args = malloc(sizeof(arg_struct));

			args->start = map;
			args->size = chunksize;
			args->index = lf_cnt;

			lf_cnt++;

			lilfinal = realloc(lilfinal, (lf_cnt+max)*sizeof(stitched));

			pthread_mutex_lock(&m);
			while (numfull == max)
				pthread_cond_wait(&empty, &m);
			do_fill(args); // questionable

			pthread_cond_signal(&fill);
			pthread_mutex_unlock(&m);
		}
		pthread_mutex_lock(&m);
		files--;
		pthread_mutex_unlock(&m);
		close(fd);
	}
	living = 0;
	pthread_cond_broadcast(&fill);
	for (int npr = 0; npr < nprocs; npr++)
	{
		pthread_join(thrds[npr], NULL);
	}

	free(queue);

	printf("\ndone\n");
	//Final stitch
	// Attempt 35 - flatten lilfinal
	// int bf_cnt = 0;
	// for (int i = 0; i < lf_cnt; ++i)
	// {
	// 	bf_cnt += lilfinal[i].numelm + 1;
	// }
	// int cnt[bf_cnt];
	// char ltr[bf_cnt];
	// int inc = 0;
	// for (int i = 0; i < lf_cnt; ++i)
	// {
	// 	for (int j = 0; lilfinal[i].letter[j] != '\0'; j++)
	// 	{
	// 		cnt[inc] = lilfinal[i].count[j];
	// 		ltr[inc] = lilfinal[i].letter[j];
	// 		inc++;
	// 	}
	// }
	// //	printf("bfcnt: (%d)\n", bf_cnt);
	// int xcnt = 0;
	// if (bf_cnt <= 1)
	// {
	// 	printf("%d",cnt[0]);
	// 	//fwrite(&cnt[0], sizeof(int), 1, stdout);
	// 	printf("%c", ltr[0]);
	// }
	// for (int i = 0; i < bf_cnt - 1; ++i)
	// {
	// 	char prev = ltr[i];
	// 	//char next = ltr[i+1];
	// 	xcnt = cnt[i];
	// 	while (ltr[i] == ltr[i + 1])
	// 	{
	// 		xcnt += cnt[i + 1];
	// 		i++;
	// 	}
	// 	//printf("%d",xcnt);
	// 	printf("%d", xcnt);
	// 	// printf("%c", prev);
	// 	// fwrite(&xcnt, sizeof(int), 1, stdout);
	// 	printf("%c", prev);
	// }

	return 0;
}
