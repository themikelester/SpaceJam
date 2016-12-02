#include <cstdint>

int socket_client_connect( const char* hostname, uint16_t port );
int socket_server_create_listener( uint16_t port );
int socket_server_accept( int listener );

int socket_send_msg( int sock, const void* data, uint32_t length );
int socket_recv_msg( int sock, void* dataOut, uint32_t length );
