#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
//function declaration
void printQ();
void pushQ(char command,int time);
int popQ();
bool emptyQ();
struct node* createQ(char command,int time);

// queue node 
struct node{
	char command;
	int time;
	struct node *next;
}*head,*tail;


//queue variables

//aggregate variable
long sum=0;
long odd=0;
long min=INT_MAX;
long max=INT_MIN;
bool done=false; // 얜 뭐에썼지
long time_param=0;//time parameter
//thread test
bool push=false;
int sleep_counter=0;
int idle_counter=0;

//pthread mutex
pthread_mutex_t mutex_1=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_2=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_3=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_4=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_create=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_1=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_2=PTHREAD_COND_INITIALIZER;



//queue test용 
void printQ(){
	struct node *temp;
	temp=head;
	while(temp!=NULL){
		printf("(%c,%d)->",temp->command,temp->time);
		temp=temp->next;
	}
}

bool emptyQ(){
	if(head==NULL)
		return true;
	else
		return false;
}


void pushQ(char command, int time){
	struct node* nNode;
	nNode=createQ(command,time);
	if(head==NULL){
		head=tail=nNode;
	}else{
		tail->next=nNode;
		tail=nNode;
	}
}

int popQ(){
	struct node *temp;
	temp=head;
	head=temp->next;
	int r=temp->time;
	free(temp);
	return r; 
}

struct node* createQ(char command,int time){
	struct node *nNode;
	nNode=(struct node*)malloc(sizeof(struct node));
	nNode->command=command;
	nNode->time=time;
	nNode->next=NULL;//NULL 매우 중요
	return nNode;
}


void* test(void *data){
	//thread가먼저 생기고 main을 돌림
	int time=0;
	
	int dt=*(int*)data;
	pthread_t tid=pthread_self();


	pthread_mutex_lock(&mutex_1);		
	pthread_cond_signal(&cond_create);
	pthread_mutex_unlock(&mutex_1);

	do{
//		printf("thread [%x] mutex이전\n",tid);		
		pthread_mutex_lock(&mutex_2);	
//		printf("thread [%x] wait이전\n",tid);
		
		idle_counter++;
		while(emptyQ() && done!=true){				
//			printf("thread[%x] sleeping!!!!!!!!!! %d \n",tid,done);					//여기들어오는게 broadcast보다 늦게 들어와버린다라
			pthread_cond_wait(&cond_1,&mutex_2);			//while해서  input이 들어올때까지 무한루프, signal 오면 다음으로 
//			printf("thread[%x] awake!!!!!!!!!!!!!!!!\n",tid);	
		}
		
		if(done){
			pthread_mutex_unlock(&mutex_2);
			break;
		}
		idle_counter--;		
//		printf("////////////pop이전://///////\n");
//		printQ();
		time=popQ();
//		printf("////////////pop이후:////////:%d\n",time);
		pthread_mutex_unlock(&mutex_2);


//		printf("thread [%x] mutex  연산 이전\n",tid);
		
		sleep(time);
	
		pthread_mutex_lock(&mutex_3);
//		printf("thread:[%x] 연산\n",tid);	
		sum+=time;
//		printf("sum: %ld \n",sum);
		if(time%2==1)
			odd++;
		if(time>max)
			max=time;
		if(time<min)
			min=time;	
		pthread_mutex_unlock(&mutex_3);
		
//		printf("thread [%x] 계산 이후 thread sleep:%d\n",tid,time);			
	}while(1);	
//	printf("thread : %x finished \n",tid);
	return (void*)0;
}

int main(int argc, char *argv[]){

	pthread_t *thread_t;
	int *status;
	if(argc != 3){
		printf("Usage: sum <input file> <number of thread>\n");
		exit(EXIT_FAILURE);
	}

	char *fn = argv[1];
	int thread_num = atoi(argv[2]);
//	printf("thread_num:%d\n",thread_num);
	thread_t=(pthread_t*)malloc(thread_num*sizeof(pthread_t));
	status=(int*)malloc(thread_num*sizeof(status));
	FILE *fin = fopen(fn,"r");

	//thread_create
	for(int i=0;i<thread_num;i++){	
		pthread_mutex_lock(&mutex_1);
		pthread_create(&thread_t[i],NULL,test,(void*)&i);//어차피 while 문 돌릴꺼니까 그냥 이렇게 해놔도 될듯 
		pthread_cond_wait(&cond_create,&mutex_1);//wait을 여러번 해야 thread가 여러번 signal보내도 충분히 가능
		pthread_mutex_unlock(&mutex_1);
	}

	char command;
	long t;
	bool end=false;
	time_t a=time(NULL);


	while(1){
		if(fscanf(fin,"%c %ld\n",&command,&t)==2){
			if(command=='p'){
				pthread_mutex_lock(&mutex_2);
				pushQ(command,t);

//				printf("//////////////////push://////////////////%c %ld\n",command,t);
//				printQ();

				pthread_cond_signal(&cond_1);			//signal이 똑바로 안되네//signal이 먼저 와서 wait을 못건드림
				pthread_mutex_unlock(&mutex_2);		
			}else if(command=='w'){
				sleep(t);	
			}
		}else{
			pthread_mutex_lock(&mutex_2);	
			if(emptyQ() && idle_counter==thread_num){     //thread모두 자고 있는것 추가 
				done=true;
				pthread_cond_broadcast(&cond_1);
//				printf("main done\n");
				pthread_mutex_unlock(&mutex_2);
				break;	
			}		
			pthread_mutex_unlock(&mutex_2);
		}	
	
	}
	for(int i=0;i<thread_num;i++){
		pthread_join(thread_t[i],(void**)&status[i]);
	
	}
	printf("sum:%ld odd:%ld max:%ld min:%ld\n",sum,odd,max,min);
	pthread_mutex_destroy(&mutex_1);

	printf("%ld\n",time(NULL)-a);	
	return 0;
}

