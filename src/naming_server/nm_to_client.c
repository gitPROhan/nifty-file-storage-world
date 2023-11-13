/**
 * @file nm_to_client.c
 * @brief Communication between the naming server and clients
 * @details
 * - Receives initial client connections and spawns a new thread for each client
 * - Handles all operations sent to the naming server from the client
 * - Forwards requests to the storage server whenever needed
 */

#include "../common/headers.h"
#include "headers.h"

extern Tree NM_Tree;

/**
 * @brief Receive path from client and send the corresponding storage server's port
 *
 * @param clientfd file descriptor of the client socket
 */
void send_client_port(const i32 clientfd)
{
  char path[MAX_STR_LEN];
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);

  const i32 port = ss_client_port_from_path(path);
  enum status code = SUCCESS;

  if (port == -1)
  {
    code = NOT_FOUND;
    CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    return;
  }

  CHECK(send(clientfd, &code, sizeof(code), 0), -1);
  CHECK(send(clientfd, &port, sizeof(port), 0), -1);
}

/**
 * @brief Receive path from client, perform specified operation on storage server and send the status code
 *
 * @param clientfd file descriptor of the client socket
 * @param op specified operation
 */
void send_nm_op_single(const i32 clientfd, const enum operation op)
{
  char path[MAX_STR_LEN];
  CHECK(recv(clientfd, path, sizeof(path), 0), -1);

  enum status code;
  // naming server is the client
  const i32 port = ss_nm_port_from_path(get_parent(path));
  if (port == -1)
  {
    code = NOT_FOUND;
    CHECK(send(clientfd, &code, sizeof(code), 0), -1);
    return;
  }
  const i32 sockfd = connect_to_port(port);

  CHECK(send(sockfd, &op, sizeof(op), 0), -1);
  CHECK(send(sockfd, path, sizeof(path), 0), -1);

  // send status code received from ss to client
  CHECK(recv(sockfd, &code, sizeof(code), 0), -1);
  CHECK(send(clientfd, &code, sizeof(code), 0), -1);

  close(sockfd);

  if (code != SUCCESS)
    return;

  if (op == CREATE_FILE)
  {
    AddFile(NM_Tree, path);
  }
  else if (op == DELETE_FILE)
  {
    DeleteFile(NM_Tree, path);
  }
  else if (op == CREATE_FOLDER)
  {
    AddFolder(NM_Tree, path);
  }
  else if (op == DELETE_FOLDER)
  {
    DeleteFolder(NM_Tree, path);
  }
}

/**
 * @brief Receive 2 paths from client, perform copy operation on storage server and send the status code
 *
 * @param clientfd file descriptor of the client socket
 * @param op specified operation
 */
void send_nm_op_double(const i32 clientfd, const enum operation op)
{
  // TODO

  (void)op;
  char from_path[MAX_STR_LEN];
  char to_path[MAX_STR_LEN];
  CHECK(recv(clientfd, from_path, sizeof(to_path), 0), -1);
  CHECK(recv(clientfd, to_path, sizeof(to_path), 0), -1);

  // naming server is the client
  const i32 from_port = ss_nm_port_from_path(from_path);
  const i32 to_port = ss_nm_port_from_path(to_path);

  const i32 from_sockfd = connect_to_port(from_port);
  const i32 to_sockfd = connect_to_port(to_port);

  // CHECK(send(sockfd, &op, sizeof(op), 0), -1);
  // CHECK(send(sockfd, path, sizeof(path), 0), -1);

  // send status code received from ss to client

  close(from_sockfd);
  close(to_sockfd);
}

/**
 * @brief Initializes connection to the clients and spawns a new client relay for each of them
 *
 * @param arg NULL
 * @return void* NULL
 */
void *client_init(void *arg)
{
  (void)arg;

  const i32 serverfd = bind_to_port(NM_CLIENT_PORT);
  printf("Listening for clients on port %i\n", NM_CLIENT_PORT);
  struct sockaddr_in client_addr;
  while (1)
  {
    socklen_t addr_size = sizeof(client_addr);
    i32 *clientfd = malloc(sizeof(i32));
    *clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
    CHECK(*clientfd, -1);

    pthread_t client_relay_thread;
    pthread_create(&client_relay_thread, NULL, client_relay, clientfd);
  }
  // maybe join client_relays if ever a break statement is added

  CHECK(close(serverfd), -1);

  return NULL;
}

/**
 * @brief Receives all operations from the client.
 * In case of READ, WRITE and METADATA, sends the port number of the corresponding storage server to the client.
 * In other cases, performs the operation and sends the status code to the client
 * @param arg integer pointer to the client file descriptor
 * @return void* NULL
 */
void *client_relay(void *arg)
{
  const i32 clientfd = *(i32 *)arg;
  free(arg);
  while (1)
  {
    enum operation op;
    CHECK(recv(clientfd, &op, sizeof(op), 0), -1)
    switch (op)
    {
    case READ:
    case WRITE:
    case METADATA:
      send_client_port(clientfd);
      break;
    case CREATE_FILE:
    case DELETE_FILE:
    case CREATE_FOLDER:
    case DELETE_FOLDER:
      send_nm_op_single(clientfd, op);
      break;
    case COPY_FILE:
    case COPY_FOLDER:
      send_nm_op_double(clientfd, op);
      break;
    }
  }

  return NULL;
}
