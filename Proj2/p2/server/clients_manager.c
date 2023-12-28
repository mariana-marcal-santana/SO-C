#include "clients_manager.h"
#include "common/constants.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void set_list_Clients(struct Client *clients) {
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    clients[i].id_session = -1 ;
    clients[i].path_request = malloc(41);
    clients[i].path_response = malloc(41);
  }
}

void set_Client(struct Client *clients, int id_session, char *path_request, char *path_response) {
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    if (i == id_session) {
      clients[i].id_session = id_session;
      strcpy(clients[i].path_request, path_request);
      strcpy(clients[i].path_response, path_response);
      break;
    }
  }
}

void remove_Client(struct Client *clients, int id_session) {
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    if (clients[i].id_session == id_session) {
      clients[i].id_session = -1;
      free(clients[i].path_request);
      free(clients[i].path_response);
      break;
    }
  }
}

int get_free_index(struct Client *clients) {
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    if (clients[i].id_session == -1) {
      return i;
    }
  }
  return -1;
}
