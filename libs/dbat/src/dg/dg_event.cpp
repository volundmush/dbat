/**************************************************************************
*  File: dg_event.c                                                       *
*                                                                         *
*  Usage: This file contains a simplified event system to allow           *
*  DG Script triggers to use the "wait" command, causing a delay in the   *
*  middle of a script.                                                    *
*                                                                         *
*  By: Mark A. Heilpern (Sammy @ eQuoria MUD   equoria.com:4000)          *
*                                                                         *
*  As of dg scripts pl 8 this includes the 'FULL' DG event package.       *                                                                       *
*  This file includes the file queue.c, which handles the priority queues.*                                                                       *
*  Thomas Arp - Welcor - 2002                                             *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
/*
 * dg_event.c: This file contains a simplified event system to allow
 * DG Script triggers to use the "wait" command, causing a delay in the
 * middle of a script.
 *
 * By: Mark A. Heilpern (Sammy @ eQuoria MUD   equoria.com:4000)
 *
 * As of dg scripts pl 8 this includes the 'FULL' DG event package.
 * This file includes the file queue.c, which handles the priority queues.
 * Thomas Arp - Welcor - 2002
 *
 */
#include <climits>

#include "dbat/dg_event.hpp"
#include "dbat/comm.hpp"
#include "dbat/log.hpp"
#include "dbat/game_world.hpp"

static void queue_deq(struct queue *q, struct q_element *qe);
static long queue_elmt_key(struct q_element *qe);
static struct q_element *queue_enq(struct queue *q, void *data, long key);
static void queue_free(struct queue *q);
static void *queue_head(GameWorld *g, struct queue *q);
static struct queue *queue_init(void);
static long queue_key(GameWorld *g, struct queue *q);


/* initializes the event queue */
void event_init(GameWorld *g)
{
  g->event_q = queue_init();
}

/*
** Add an event to the current list
*/
/* creates an event and returns it */
struct event *event_create(GameWorld *g, EVENTFUNC(*func), void *event_obj, long when)
{
  struct event *new_event;

  if (when < 1) /* make sure its in the future */
    when = 1;

  CREATE(new_event, struct event, 1);
  new_event->func      = func;
  new_event->event_obj = event_obj;
  new_event->q_el      = queue_enq(g->event_q, new_event, when + g->pulse);

  return new_event;
}

/* removes the event from the system */
void event_cancel(GameWorld *g, struct event *event)
{
  if (!event) {
    mud_log("SYSERR:  Attempted to cancel a NULL event");
    return;
  }

  if (!event->q_el) {
    mud_log("SYSERR:  Attempted to cancel a non-NULL unqueued event, freeing anyway");
  } else
    queue_deq(g->event_q, event->q_el);

  if (event->event_obj)
    free(event->event_obj);
  free(event);
}

/* Process any events whose time has come. */
void event_process(GameWorld *g)
{
  struct event *the_event;
  long new_time;

  while ((long)g->pulse >= queue_key(g, g->event_q)) {
    if (!(the_event = (struct event *)queue_head(g, g->event_q))) {
      mud_log("SYSERR: Attempt to get a NULL event");
      return;
    }

    /*
    ** Set the_event->q_el to NULL so that any functions called beneath
    ** event_process can tell if they're being called beneath the actual
    ** event function.
    */
    the_event->q_el = nullptr;

    /* call event func, reenqueue event if retval > 0 */
    if ((new_time = (the_event->func)(g, the_event->event_obj)) > 0)
      the_event->q_el = queue_enq(g->event_q, the_event, new_time + g->pulse);
    else
      free(the_event);
  }
}

/* returns the time remaining before the event */
long event_time(GameWorld *g, struct event *event)
{
  long when;

  when = queue_elmt_key(event->q_el);

  return (when - g->pulse);
}

/* frees all events in the queue */
void event_free_all(GameWorld *g)
{
  queue_free(g->event_q);
  g->event_q = nullptr;
}

/* ************************************************************************
*  File: queue.c                                                          *
*                                                                         *
*  Usage: generic queue functions for building and using a priority queue *
*                                                                         *
************************************************************************ */

/* returns a new, initialized queue */
static struct queue *queue_init(void)
{
  struct queue *q;

  CREATE(q, struct queue, 1);

  return q;
}

/* add data into the priority queue q with key */
static struct q_element *queue_enq(struct queue *q, void *data, long key)
{
  struct q_element *qe, *i;
  int bucket;

  CREATE(qe, struct q_element, 1);
  qe->data = data;
  qe->key  = key;

  bucket = key % NUM_EVENT_QUEUES; /* which queue does this go in */

  if (!q->head[bucket]) { /* queue is empty */
    q->head[bucket] = qe;
    q->tail[bucket] = qe;
  }

  else {
    for (i = q->tail[bucket]; i; i = i->prev) {

      if (i->key < key) { /* found insertion point */
        if (i == q->tail[bucket])
          q->tail[bucket] = qe;
        else {
          qe->next      = i->next;
          i->next->prev = qe;
        }

        qe->prev = i;
        i->next  = qe;
        break;
      }
    }

    if (i == nullptr) { /* insertion point is front of list */
      qe->next        = q->head[bucket];
      q->head[bucket] = qe;
      qe->next->prev  = qe;
    }
  }

  return qe;
}

/* remove queue element qe from the priority queue q */
static void queue_deq(struct queue *q, struct q_element *qe)
{
  int i;

  i = qe->key % NUM_EVENT_QUEUES;

  if (qe->prev == nullptr)
    q->head[i] = qe->next;
  else
    qe->prev->next = qe->next;

  if (qe->next == nullptr)
    q->tail[i] = qe->prev;
  else
    qe->next->prev = qe->prev;

  free(qe);
}

/*
 * removes and returns the data of the
 * first element of the priority queue q
 */
static void *queue_head(GameWorld *g, struct queue *q)
{
  void *dg_data;
  int i;

  i = g->pulse % NUM_EVENT_QUEUES;

  if (!q->head[i])
    return nullptr;

  dg_data = q->head[i]->data;
  queue_deq(q, q->head[i]);
  return dg_data;
}

/*
 * returns the key of the head element of the priority queue
 * if q is NULL, then return the largest unsigned number
 */
static long queue_key(GameWorld *g, struct queue *q)
{
  int i;

  i = g->pulse % NUM_EVENT_QUEUES;

  if (q->head[i])
    return q->head[i]->key;
  else
    return LONG_MAX;
}

/* returns the key of queue element qe */
static long queue_elmt_key(struct q_element *qe)
{
  return qe->key;
}

/* free q and contents */
static void queue_free(struct queue *q)
{
  int i;
  struct q_element *qe, *next_qe;
  struct event *event;

  if (q == nullptr)
    return;

  for (i = 0; i < NUM_EVENT_QUEUES; i++)
    for (qe = q->head[i]; qe; qe = next_qe) {
      next_qe = qe->next;
      if ((event = (struct event *)qe->data) != nullptr) {
        if (event->event_obj)
          free(event->event_obj);
        free(event);
      }
      free(qe);
    }

  free(q);
}
