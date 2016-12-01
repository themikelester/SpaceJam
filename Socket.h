#include <cstdint>

int client_connect( uint16_t port );
int server_create_listener( uint16_t port );
int server_accept( int listener );

int socket_send( int sock, const void* data, uint32_t length );
int socket_recv( int sock, void* dataOut, uint32_t length );
