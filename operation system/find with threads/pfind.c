#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fnmatch.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>

struct sigaction sa;

typedef struct queue{
		char path[100];
		struct queue *next;
}queue;

struct queue *head, *tail, *save;
pthread_mutex_t lock_queue, lock_dir;
pthread_cond_t notEmpty;
unsigned int numOfActiveThreads;
pthread_t* tids;
int numOfThread;
int count_file = 0;


void sig_handler()
{
	while (head)
	{
		queue* save = head;
		head = head->next;
		free(save);
	}
	for (int i =0 ;i < numOfThread;++i)
	{
		pthread_cancel(tids[i]);
	}
	pthread_cond_destroy(&notEmpty);
	pthread_mutex_destroy(&lock_queue);
	pthread_mutex_destroy(&lock_dir );
	printf("%d: files has been founded", count_file);
	exit(1);
}


int name_corresponds(const char *path, const char *str)
{
	return fnmatch(str, strrchr(path,'/')+1, 0) == 0;
}

void* traversal(void* argv)
{
	char* term = argv;
	char dir_name[100];
	DIR * dir;
	struct dirent *entry;
	struct stat dir_stat;
	while (true)
	{
		pthread_mutex_lock(&lock_queue);
		__sync_fetch_and_sub(&numOfActiveThreads, 1);
		while (head == NULL)
		{		
			pthread_testcancel();
			if (numOfActiveThreads == 0)
			{
				pthread_mutex_unlock(&lock_queue);
				pthread_cond_signal(&notEmpty);
				pthread_exit(NULL);
			}
			pthread_testcancel();
			pthread_cond_wait(&notEmpty, &lock_queue);
			pthread_testcancel();
		}
		__sync_fetch_and_add(&numOfActiveThreads, 1);
		if (head == tail)
		{
			tail = NULL;
		}
		strcpy(dir_name, head->path);
		save = head;
		head = head->next;
		free(save);
		pthread_mutex_unlock(&lock_queue);
		dir = opendir(dir_name);
		if (! dir) { perror(dir_name); exit(1); }
		pthread_mutex_lock(&lock_dir);
		while((entry = readdir(dir)) != NULL)
		{
			pthread_mutex_unlock(&lock_dir);
			char buff[strlen(dir_name)+strlen(entry->d_name)+2];
       		sprintf(buff,"%s/%s", dir_name, entry->d_name);
			stat(buff, &dir_stat);
			if ((strcmp(entry->d_name,"..") != 0) && (strcmp(entry->d_name,".") != 0))
			{
				if ((dir_stat.st_mode & __S_IFMT) == __S_IFDIR)
				{
					pthread_mutex_lock(&lock_queue);
					pthread_cond_signal(&notEmpty);
					if (tail == NULL)
					{
						head = malloc(sizeof(queue));
						strcpy(head->path, buff);
						tail = head;
						head->next = NULL;
					}
					else
					{
						tail->next = malloc(sizeof(queue));
						tail = tail->next;
						strcpy(tail->path, buff);
						tail->next = NULL;
					}
					pthread_mutex_unlock(&lock_queue);
				}
				else
				{
					 if (name_corresponds(buff,term)) 
					{
						count_file++;
						printf("%s\n", buff);
					}
				}
			 }	
			pthread_mutex_lock(&lock_dir);
		}
		pthread_mutex_unlock(&lock_dir);
		closedir(dir);
	}
}


int main(int argc, char **argv)
{ 
	sa.sa_handler = sig_handler;
	sigaction(SIGINT, &sa, NULL);
	if(argc < 4)
	{	
		exit(1);
	}
	char *root = argv[1];
	char term[strlen(argv[2]) + 2];
	strcpy(term, "*");
	strcat(term, argv[2]);
	strcat(term, "*");
	numOfThread = atoi(argv[3]);
	head = malloc(sizeof(queue));
	numOfActiveThreads = numOfThread;
	strcpy(head->path, root);
	head->next = NULL;
	tail = head;
	tids = malloc(numOfThread * sizeof(pthread_t));
	pthread_mutex_init( &lock_dir,  NULL );
	pthread_mutex_init( &lock_queue,  NULL );
	pthread_cond_init(&notEmpty, NULL);
	for ( int i = 0;i < numOfThread;++i)
	{
		pthread_create(&tids[i], NULL, traversal, term);
	}	
	for (int i = 0;i < numOfThread;++i)
	{
		pthread_join(tids[i], NULL);	
	}
	pthread_cond_destroy(&notEmpty);
	pthread_mutex_destroy( &lock_queue );
	pthread_mutex_destroy( &lock_dir );
	printf("%d: files has been founded", count_file);
}
