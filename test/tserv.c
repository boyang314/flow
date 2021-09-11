#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /*
    struct sockaddr sin = { 0 };
    bind(listenfd, &sin, sizeof(sin));
    */

    struct sockaddr_in sin = { 0 };
    //sin.sin_family = AF_INET; //getsockname shows sin_family==2/AF_INET without this assignment
    sin.sin_port = htons(8080);
    bind(listenfd, (struct sockaddr*)&sin, sizeof(sin));

    socklen_t len = sizeof(sin);
    if (getsockname(listenfd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else {
        printf("family %d\n", sin.sin_family);
        printf("addr %s\n", inet_ntoa(sin.sin_addr));
        printf("port number %d\n", ntohs(sin.sin_port));
    }

    listen(listenfd, 10);
    while (1) {
        int connfd = accept(listenfd, NULL, NULL);
        printf("got connection %d\n", connfd);
    }
}
