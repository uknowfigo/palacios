/* 
 * This file is part of the Palacios Virtual Machine Monitor developed
 * by the V3VEE Project with funding from the United States National 
 * Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  You can find out more at 
 * http://www.v3vee.org
 *
 * Copyright (c) 2008, Jack Lange <jarusl@cs.northwestern.edu> 
 * Copyright (c) 2008, The V3VEE Project <http://www.v3vee.org> 
 * All rights reserved.
 *
 * Author: Jack Lange <jarusl@cs.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "V3VEE_LICENSE".
 */

#include <palacios/vmm.h>
#include <palacios/vmm_host_events.h>


int v3_init_host_events(struct guest_info * info) {
  struct v3_host_events * host_evts = &(info->host_event_hooks);

  INIT_LIST_HEAD(&(host_evts->keyboard_events));
  INIT_LIST_HEAD(&(host_evts->mouse_events));
  INIT_LIST_HEAD(&(host_evts->timer_events));

  return 0;
}


int v3_hook_host_event(struct guest_info * info, 
		       v3_host_evt_type_t event_type, 
		       union v3_host_event_handler cb, 
		       void * private_data) {
  
  struct v3_host_events * host_evts = &(info->host_event_hooks);
  struct v3_host_event_hook * hook = NULL;

  hook = (struct v3_host_event_hook *)V3_Malloc(sizeof(struct v3_host_event_hook));
  if (hook == NULL) {
    PrintError("Could not allocate event hook\n");
    return -1;
  }

  hook->cb = cb;
  hook->private_data = private_data;

  switch (event_type)  {
  case HOST_KEYBOARD_EVT:
    list_add(&(hook->link), &(host_evts->keyboard_events));
    break;
  case HOST_MOUSE_EVT:
    list_add(&(hook->link), &(host_evts->mouse_events));
    break;
  case HOST_TIMER_EVT:
    list_add(&(hook->link), &(host_evts->timer_events));
    break;
  }

  return 0;
}


int v3_deliver_keyboard_event(struct guest_info * info, 
			      struct v3_keyboard_event * evt) {
  struct v3_host_events * host_evts = &(info->host_event_hooks);
  struct v3_host_event_hook * hook = NULL;

  list_for_each_entry(hook, &(host_evts->keyboard_events), link) {
    if (hook->cb.keyboard_handler(info, evt, hook->private_data) == -1) {
      return -1;
    }
  }

  return 0;
}


int v3_deliver_mouse_event(struct guest_info * info, 
			   struct v3_mouse_event * evt) {
  struct v3_host_events * host_evts = &(info->host_event_hooks);
  struct v3_host_event_hook * hook = NULL;

  list_for_each_entry(hook, &(host_evts->mouse_events), link) {
    if (hook->cb.mouse_handler(info, evt, hook->private_data) == -1) {
      return -1;
    }
  }

  return 0;
}


int v3_deliver_timer_event(struct guest_info * info, 
			   struct v3_timer_event * evt) {
  struct v3_host_events * host_evts = &(info->host_event_hooks);
  struct v3_host_event_hook * hook = NULL;

  list_for_each_entry(hook, &(host_evts->timer_events), link) {
    if (hook->cb.timer_handler(info, evt, hook->private_data) == -1) {
      return -1;
    }
  }

  return 0;
}
