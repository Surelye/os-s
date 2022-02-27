#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <ucontext.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum {
  BM_ITER,
  BM_REC,
  BM_REC_ITER,
} brute_mode_t;

typedef enum {
  RM_SINGLE,
  RM_MULTI,
  RM_GENERATOR,
} run_mode_t;

typedef struct config_t {
  char * alph;
  char * hash;
  int length;
  brute_mode_t brute_mode;
  run_mode_t run_mode;
} config_t;

#define PASSWORD_MAX_LENGTH (8)
typedef char password_t[PASSWORD_MAX_LENGTH];

typedef struct task_t {
  password_t password;
  int from, to;
} task_t;

typedef struct queue_t {
  task_t queue[8];
  int head, tail;
  pthread_mutex_t head_mutex, tail_mutex;
  sem_t empty, full;
} queue_t;

typedef struct mt_context_t {
  password_t password;
  config_t * config;
  queue_t queue;
  volatile int tip;
  pthread_mutex_t tip_mutex;
  pthread_cond_t tip_cond;
} mt_context_t;

typedef bool (*password_handler_t) (task_t * task, void * context);

typedef struct check_password_context_t {
  char * hash;
  struct crypt_data cd;
} check_password_context_t;

typedef struct iter_state_t {
  task_t * task;
  int idx[PASSWORD_MAX_LENGTH];
  char * alph;
  int alph_length_1;
} iter_state_t;

#define STACKSIZE 131072 // MINSIGSTKSZ

typedef struct rec_state_t {
  ucontext_t main, rec;
  char stack[STACKSIZE];
  bool finished;
} rec_state_t;

typedef struct gn_context_t {
  password_t password;
  config_t * config;
  bool has_next;
  pthread_mutex_t mutex;
  iter_state_t iter_state;
} gn_context_t;

void queue_init (queue_t * queue) {
  queue->head = 0;
  queue->tail = 0;
  pthread_mutex_init (&queue->head_mutex, NULL);
  pthread_mutex_init (&queue->tail_mutex, NULL);
  sem_init (&queue->full, 0, 0);
  sem_init (&queue->empty, 0, sizeof (queue->queue) / sizeof (queue->queue[0]));
}

void queue_push(queue_t * queue, task_t * task)
{
  sem_wait (&queue->empty);
  pthread_mutex_lock (&queue->tail_mutex);
  queue->queue[queue->tail] = *task;
  if (++queue->tail >= sizeof (queue->queue) / sizeof (queue->queue[0]))
    queue->tail = 0;
  pthread_mutex_unlock (&queue->tail_mutex);
  sem_post (&queue->full);
}

void queue_pop (queue_t * queue, task_t * task)
{
  sem_wait (&queue->full);
  pthread_mutex_lock (&queue->head_mutex);
  *task = queue->queue[queue->head];
  if (++queue->head >= sizeof (queue->queue) / sizeof (queue->queue[0]))
    queue->head = 0;
  pthread_mutex_unlock (&queue->head_mutex);
  sem_post (&queue->empty);
}

bool check_password (char * password, char * hash, struct crypt_data * cd)
{
  char * _hash = crypt_r (password, hash, cd);
  return (strcmp (_hash, hash) == 0);
}

bool check_password_handler (task_t * task, void * context)
{
  check_password_context_t * check_password_context = context;
  char * _hash = crypt_r (task->password, check_password_context->hash, &check_password_context->cd);
  return (strcmp (_hash, check_password_context->hash) == 0);
}

bool brute_rec (task_t * task, char * alph, password_handler_t password_handler, void * context, int pos)
{
  int i;
  if (pos == task->to)
    return (password_handler (task, context));

  for (i = 0; alph[i]; ++i)
    {
      task->password[pos] = alph[i];
      if (brute_rec (task, alph, password_handler, context, pos + 1))
        return (true);
    }
  
 return (false);
}

bool brute_rec_wrapper (task_t * task, char * alph, password_handler_t password_handler, void * context)
{
  return (brute_rec (task, alph, password_handler, context, task->from));
}

bool brute_rec_coop_handler (task_t * task, void * context)
{
  rec_state_t * rec_state = context;
  swapcontext (&rec_state->rec, &rec_state->main);

  return (false);
}

void brute_rec_coop (task_t * task, char * alph, rec_state_t * rec_state)
{
  rec_state->finished = false;
  brute_rec_wrapper (task, alph, brute_rec_coop_handler, rec_state);
  rec_state->finished = true;
}

void rec_state_init (rec_state_t * rec_state, task_t * task, char * alph)
{
  getcontext (&rec_state->main);
  rec_state->rec = rec_state->main;
  rec_state->rec.uc_stack.ss_sp = rec_state->stack;
  rec_state->rec.uc_stack.ss_size = sizeof (rec_state->stack);
  rec_state->rec.uc_link = &rec_state->main;
  makecontext (&rec_state->rec, (void (*) (void))brute_rec_coop, 3, task, alph, rec_state);
  swapcontext (&rec_state->main, &rec_state->rec);
}

bool rec_state_next (rec_state_t * rec_state)
{
  swapcontext (&rec_state->main, &rec_state->rec);
  return (!rec_state->finished);
}

void iter_state_init (iter_state_t * iter_state, task_t * task, char * alph)
{
  int i;
  
  iter_state->alph_length_1 = strlen(alph) - 1;
  iter_state->task = task;
  iter_state->alph = alph;
  
  for (i = task->from; i < task->to; ++i)
    {
      iter_state->idx[i] = 0;
      task->password[i] = alph[0];
    }
}

bool brute_rec_iter (task_t * task, char * alph, password_handler_t password_handler, void * context)
{
  rec_state_t rec_state;
  rec_state_init (&rec_state, task, alph);
  
  for(;;)
    {
      if (password_handler (task, context))
	return(true);
      if (!rec_state_next (&rec_state))
	break;
    }

  printf ("Hello from brute_rec_iter.\n");
  return (false);
}

bool iter_state_next (iter_state_t * iter_state)
{
  int i;
  task_t * task = iter_state->task;
  
  for (i = task->to - 1; (i >= task->from) && (iter_state->idx[i] == iter_state->alph_length_1); --i)
    {
      iter_state->idx[i] = 0;
      task->password[i] = iter_state->alph[0];
    }
  
  if (i < task->from)
    return (false);
  
  task->password[i] = iter_state->alph[++iter_state->idx[i]];
  
  return (true);
}

bool brute_iter (task_t * task, char * alph, password_handler_t password_handler, void * context)
{
  iter_state_t iter_state;
  iter_state_init (&iter_state, task, alph);
  
  for(;;)
    {
      if (password_handler (task, context))
	return(true);
      if (!iter_state_next (&iter_state))
	break;
    }
  
  return (false);
}

void parse_params (config_t * config, int argc, char * argv[]) {
  int opt;
  while ((opt = getopt (argc, argv, "smgirea:l:h:c:")) != -1)
    switch (opt)
      {
      case 'c':
        printf ("%s\n", crypt (optarg, "aa"));
        exit (EXIT_FAILURE);
      case 's':
        config->run_mode = RM_SINGLE;
        break;
      case 'm':
        config->run_mode = RM_MULTI;
        break;
      case 'g':
        config->run_mode = RM_GENERATOR;
        break;
      case 'i':
        config->brute_mode = BM_ITER;
        break;
      case 'r':
        config->brute_mode = BM_REC;
        break;	
      case 'e':
	config->brute_mode = BM_REC_ITER;
	break;
      case 'a':
	config->alph = optarg;
        break;
      case 'h':
        config->hash = optarg;
        break;
      case 'l':
        config->length = atoi (optarg);
        break;
    }
}

bool process_task (task_t * task, config_t * config, password_handler_t password_handler, void * context)
{
  bool found = false;

  switch (config->brute_mode)
    {
    case BM_REC:
      found = brute_rec_wrapper (task, config->alph, password_handler, context);
      break;

    case BM_ITER:
      found = brute_iter (task, config->alph, password_handler, context);
      break;
      
    case BM_REC_ITER:
      found = brute_rec_iter (task, config->alph, password_handler, context);
      break;
    }

  return (found);
}

bool run_single (task_t * task, config_t * config)
{
  check_password_context_t check_password_context;
  
  check_password_context.hash = config->hash;
  check_password_context.cd.initialized = 0;
  
  return (process_task (task, config, check_password_handler, &check_password_context));
}

void * mt_worker (void * arg)
{
  mt_context_t * mt_context = arg;
  check_password_context_t check_password_context;
  
  check_password_context.hash = mt_context->config->hash;
  check_password_context.cd.initialized = 0;

  for (;;)
    {
      task_t task;
      queue_pop (&mt_context->queue, &task);

      task.to = task.from;
      task.from = 0;
      
      if (process_task (&task, mt_context->config, check_password_handler, &check_password_context))
	memcpy (mt_context->password, &task.password, sizeof (task.password));
      
      pthread_mutex_lock(&mt_context->tip_mutex);
      --mt_context->tip;
      pthread_mutex_unlock(&mt_context->tip_mutex);
      if (mt_context->tip == 0)
	pthread_cond_signal(&mt_context->tip_cond);
    }
}

bool mt_password_handler (task_t * task, void * context)
{
  mt_context_t * mt_context = context;

  pthread_mutex_lock (&mt_context->tip_mutex);
  ++mt_context->tip;
  pthread_mutex_unlock (&mt_context->tip_mutex);

  queue_push (&mt_context->queue, task);
  
  return (mt_context->password[0] != 0);
}

bool run_multi (task_t * task, config_t * config)
{
  long ncpu = sysconf (_SC_NPROCESSORS_ONLN);
  mt_context_t mt_context;
  pthread_t ids[ncpu];
  int i;
  
  task->from = 2;
  queue_init (&mt_context.queue);
  mt_context.config = config;
  mt_context.password[0] = 0;
  mt_context.tip = 0;
  pthread_mutex_init (&mt_context.tip_mutex, NULL);
  pthread_cond_init (&mt_context.tip_cond, NULL);
  
  for (i = 0; i < ncpu; ++i)
    pthread_create (&ids[i], NULL, mt_worker, &mt_context);
    
  process_task (task, config, mt_password_handler, &mt_context);

  pthread_mutex_lock (&mt_context.tip_mutex);
  while (mt_context.tip != 0)
      pthread_cond_wait (&mt_context.tip_cond, &mt_context.tip_mutex);
  pthread_mutex_unlock (&mt_context.tip_mutex);

  for (i = 0; i < ncpu; ++i)
    {
      pthread_cancel (ids[i]);
      pthread_join (ids[i], NULL);
    }
  
  memcpy (task->password, mt_context.password, sizeof (mt_context.password));
  
  return (task->password[0] != 0);
}

void * gn_worker (void * arg)
{
  gn_context_t * gn_context = arg;
  check_password_context_t check_password_context;

  check_password_context.hash = gn_context->config->hash;
  check_password_context.cd.initialized = 0;

  for (;;)
    {
      bool finished = false;
      task_t task;
      pthread_mutex_lock (&gn_context->mutex);
      task = *gn_context->iter_state.task;
      if (gn_context->has_next)
	gn_context->has_next = iter_state_next (&gn_context->iter_state);
      else
	finished = true;
      pthread_mutex_unlock (&gn_context->mutex);

      if (finished)
	break;

      task.to = task.from;
      task.from = 0;

      if (process_task (&task, gn_context->config, check_password_handler, &check_password_context))
	{
	  pthread_mutex_lock (&gn_context->mutex);
	  gn_context->has_next = false;
	  pthread_mutex_unlock (&gn_context->mutex);
	  memcpy (gn_context->password, &task.password, sizeof (task.password));
	}
    }

  return (NULL);
}

bool run_generator (task_t * task, config_t * config)
{
  long ncpu = sysconf (_SC_NPROCESSORS_ONLN) - 1;
  gn_context_t gn_context;
  pthread_t ids[ncpu];
  int i;
  
  task->from = 2;
  gn_context.config = config;
  gn_context.has_next = true;
  iter_state_init (&gn_context.iter_state, task, config->alph);
  pthread_mutex_init (&gn_context.mutex, NULL);
  gn_context.password[0] = 0;
  
  for (i = 1; i < ncpu; ++i)
    pthread_create (&ids[i], NULL, gn_worker, &gn_context);

  gn_worker (&gn_context);

  for (i = 0; i < ncpu; ++i) 
    pthread_join (ids[i], NULL);
  
  memcpy (task->password, gn_context.password, sizeof (gn_context.password));
  
  return (task->password[0] != 0);
}

int main (int argc, char * argv[])
{
  config_t config = {
     .alph = "0123456789abc",
     .hash = "aa88txHygpR5g",
     .length = 3,
     .brute_mode = BM_ITER,
     .run_mode = RM_SINGLE,
    };
  
  parse_params(&config, argc, argv);
  
  bool found = false;
  task_t task;
  
  task.password[config.length] = 0;
  task.from = 0;
  task.to = config.length;
  
  switch (config.run_mode)
    {
    case RM_SINGLE:
      found = run_single (&task, &config);
      break;
    case RM_MULTI:
      found = run_multi (&task, &config);
      break;
    case RM_GENERATOR:
      found = run_generator (&task, &config);
      break;
    }
  
  if (!found)
    printf ("Password is '%s'.\n", task.password);
  else
    printf ("Password not found.\n");
     
  return (EXIT_SUCCESS);   
}
