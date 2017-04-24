#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include <errno.h>
//pour le client
#include <netdb.h>
#include <sys/select.h>

#define MAXREQUESTSIZE 4096

/* ================================================ HELP ================================================ */
void showHelp(){
	printf("HELP Proxy : Proxy <n_Port>\n");
}

/* ================================================ MAIN ================================================ */
int main(int argc,char *argv[]){
	
	int sockfd;
	int newsockfd;

	int i;

	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;

	socklen_t clilen;


	// Il faut seulement un numero de port vers lequel ecouter
	if(argc != 2){
		showHelp();
		exit(1);
	}

	// DEBUG d'aide
	printf("================================ WELCOME to ADBLOCK/PROXY SERVER ================================\n");
	printf("HELP Proxy : Pour se connecter en client, dans un autre terminal faire 'tellnet localhost n_Port'\n");
	printf("HELP Proxy : Pour quitter un client faire 'ctrl+altgr+] => enter => q => enter'\n");
	printf("HELP Proxy : Pour quitter le serveur (ici) faire 'ctrl+c'\n");
	printf("=================================================================================================\n");

	// Creation de la socket de dialogue entre le navigateur et le proxy
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) <0) {
		printf("ERROR Proxy: Erreur lors de la creation de la socket de dialogue !\n");
		exit (1);
	}

	// Bind
	memset( (char*) &serv_addr,0, sizeof(serv_addr) );

	serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(atoi(argv[1]));
        serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr) ) <0) {
		printf("ERROR Proxy: Erreur lors du bind, attendre un peu ou utiliser un autre nPort !\n");
		exit (1);
	}

	// Listen
	if (listen(sockfd, SOMAXCONN) < 0) {
		printf("ERROR Proxy: Erreur lors du listen !\n");
		exit (1);
	}

	bzero((char * ) & cli_addr, sizeof(cli_addr));

	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr * ) & cli_addr, & clilen);

	char clientRequest[MAXREQUESTSIZE];
	bzero((char * ) & clientRequest, sizeof(clientRequest));

	recv(newsockfd, clientRequest, sizeof(clientRequest), 0);

	printf("DEBUG Proxy: Requette\n");
	printf("%s\n",clientRequest);
	printf("DEBUG Proxy: End Requetten");

	//parsing
	printf("DEBUG Proxy: Parsing to get host...\n");
	char *token;
	const char delimiter[] = " \r\n";
	char* host;
	// on copie la requette sinon elle est modifiee par strtok
	char clientRequestCopy[MAXREQUESTSIZE];
	strcpy(clientRequestCopy,clientRequest);
	// On recupere split[0] dans token
	token = strtok(clientRequestCopy, delimiter);

	// Token contient split[i] successivement jusqu'au dernier ou il vaut null
	while( strcmp(token, "Host:") != 0){
		//printf( "%s / %d\n", token, strlen(token) );
		token = strtok(NULL, delimiter);
	}
	token = strtok(NULL, delimiter);
	host = (char*)malloc(strlen(token)*sizeof(char));
	strcpy(host,token);

	while( strtok(NULL, delimiter) != NULL){
	}
	printf("DEBUG Proxy: Parsing (%s) OK!\n\n",host);

	printf("DEBUG Proxy: Parsing to get cookie...\n");
	char* cookie;

	strcpy(clientRequestCopy,clientRequest);
	// On recupere split[0] dans token

	const char delimiter_cookie[] = "\r\n";
	token = strtok(clientRequestCopy, delimiter_cookie);
	
	//Recupere la premiere ligne
	char* get;
	get = (char*)malloc(strlen(token)*sizeof(char));
	strcpy(get,token);

	while( strstr(token, "Cookie: ") == NULL){
		token = strtok(NULL, delimiter_cookie);
	}
	cookie = (char*)malloc(strlen(token)*sizeof(char));
	strcpy(cookie,token);


	//Connection a l'host
	struct sockaddr_in host_addr;
	struct hostent * hostStruct;

	hostStruct = gethostbyname(host);

	bzero((char * ) & host_addr, sizeof(host_addr));
	host_addr.sin_port = htons(80);
	host_addr.sin_family = AF_INET;
	bcopy((char * ) hostStruct -> h_addr, (char * ) & host_addr.sin_addr.s_addr, hostStruct -> h_length);

	int sockfd1;
	int newsockfd1;
	
	// Creation de la socket de dialogue le proxy et l'internet
	if ((sockfd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ) <0) {
		printf("ERROR Proxy: Erreur lors de la creation de la socket de dialogue deuxieme du nom!\n");
	}

	newsockfd1 = connect(sockfd1, (struct sockaddr * ) & host_addr, sizeof(struct sockaddr));
	if (newsockfd1 < 0)
		printf("Error in connecting to remote server");

	// Affiche l'ip du host
	char buffer[50];
	sprintf(buffer, "\nConnected to %s  IP - %s\n", host, inet_ntoa(host_addr.sin_addr));
	printf("\n%s\n", buffer);

	// Creation de la requettttttttttte a envoyer
	char requestToSend[MAXREQUESTSIZE];
	bzero((char * ) requestToSend, sizeof(requestToSend));

	sprintf(requestToSend, "%s\r\nHost: %s\r\n%s\r\nConnection: close\r\n\r\n", get, host,cookie);
	printf("RrequestToSend\n%s\n", requestToSend);
	

	int nbOctetSend;
	nbOctetSend = send(sockfd1, requestToSend, strlen(requestToSend), 0);

	char html[500];
	bzero((char * ) html, sizeof(html));

	if (nbOctetSend < 0)
		printf("Error writing to socket\n %s\n", strerror(errno));
	else {
                do {
                        bzero((char * ) html, sizeof(html));
			printf("wait for receive %d\n", nbOctetSend);
                        nbOctetSend = recv(sockfd1, html, sizeof(html), 0);
                        if ( !(nbOctetSend <= 0) )
                                send(newsockfd, html, nbOctetSend, 0);
                } while (nbOctetSend > 0);
        }

       //TODO close
	

	return 0;

}
