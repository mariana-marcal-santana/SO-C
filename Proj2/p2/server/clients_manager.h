#ifndef CLIENTS_MANAGER_H
#define CLIENTS_MANAGER_H

#include <pthread.h>

struct Client {
  int id_session;
  char *path_request;
  char *path_response;
  pthread_t thread;
};

void set_list_Clients(struct Client *clients) ;
void set_Client(struct Client *clients, int id_session, char *path_request, char *path_response);
void remove_Client(struct Client *clients, int id_session);
int get_free_index(struct Client *clients) ;

#endif // CLIENTS_MANAGER_H