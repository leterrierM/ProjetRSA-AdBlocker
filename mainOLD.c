#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.c"

//pour le client
#include <netdb.h>
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

#include <sys/select.h>

int echoToClient(int sockfd, char* msg, int nrcv){
	int nsnd;
	if ( (nsnd = write (sockfd, msg, nrcv) ) <0 ) {
		printf("ERROR Proxy: Erreur lors de l'ecriture sur le socket %d !\n",sockfd);
		exit (1);
	}
	printf("DEBUG Proxy: Renvoie du message de taille %d sur la socket %d !\n",nsnd,sockfd);
	return( nsnd );
}

#define BUFSIZE 1500
#define MAXLINE 80
#define CHUNK_SIZE 512
int strRequest(int sockfd){
	// contient le nb d'octet lu => pointe juste apres le dernier caractere dans msg on peut donc y mettre \0
	int nrcv;
	int nsnd;
	char msg[BUFSIZE];

	memset( (char*) msg, 0, sizeof(msg) );
	if ( (nrcv = read ( sockfd, msg, sizeof(msg)-1) ) < 0 )  {
		printf("ERROR Proxy: Erreur lors de la lecture sur le socket %d !\n",sockfd);
		exit (1);
	}
	msg[nrcv]='\0';

	printf("DEBUG Proxy: Message de taille %d recu sur le socket %d du processus %d  !\n",nrcv,sockfd,getpid());
	printf("DEBUG Proxy: Debut du message de la socket %d :\n",sockfd);
	printf("%s",msg);
	printf("DEBUG Proxy: Fin du message ===================\n");

	// TODO A changer pour seulement check si la co est fermé par le client
	nsnd = echoToClient(sockfd, msg, nrcv);


	/*
	//TODO parse
	aide pour parser
	strcpy(str1,str2)
	(strncmp(str,"GET",3)==0) 3 pour 3 caract renvoie 0 si les chaines sont les memes
	strlen(str)
	strcat(str1,str2);
	strtok(str,"split"); c'est un split et ca renvoie surement un char*
	attention strtok() modifie son premier argument.
	Les caractères de séparation sont surchargés, leur identité est donc perdue.
	Cette fonction ne doit pas être invoquée sur une chaîne constante.
	La fonction strtok() utilise un tampon statique et n'est donc pas sûre dans un contexte multithread. 
	Dans ce cas, il vaut mieux utiliser strtok_r().
	*/
	// TODO Peut etre faire une methode qui renvoie un char** tableau de chaine de char
	printf("DEBUG Proxy: TEST strtok\n");

	// allocate space for GET, HOST, User-Agent, Accept,Accept-Language, Accept-Encoding, Connection, Cache-Control
	// A voir si autre cookie + post par exemple
	// Plus si ya trop d'info ca fait segfault
	// en fait j'ai l'impression ya besoin de juste les 2 premiere ligne sachant que les autres sont optionnel...
	char **request = (char**)malloc(10*sizeof(char*));
	int i = 0;
	// Allocate space for char in the line
	for(i = 0; i < 5; i++){
		request[i] = (char*)malloc(250*sizeof(char));
	}

	char *token;
	const char delimiter[2] = "\n";
	// On recupere spit[0] dans token
	token = strtok(msg, delimiter);

	// Token contient split[i] successivement jusqu'au dernier ou il vaut null
	i = 0;
	while( token != NULL ){
		printf( "%s\n", token );
		request[i] = token;
		token = strtok(NULL, delimiter);
		i++;
	}
	for(i=0; i < sizeof(request); i++){
		printf("%s\n",request[i]);
	}
	
	printf("DEBUG Proxy: FIN TEST strtok=============\n");

	//TODO requette a internet pour recup du html et le rebalance au client
	printf("\nDEBUG Proxy: TEST recuperation de l'html et renvoie au client\n");

	char arg[500];
	char firstHalf[500];
	char secondHalf[500];
	char requestEnvoye[1000];
	char bufferReceive[15000];
	struct hostent *server;
	struct sockaddr_in serveraddr;
	int port = 80;

	strcpy(arg, "lamport.azurewebsites.net/tla/tla.html");
	//strcpy(arg, "arche.univ-lorraine.fr/my/");
	for (i = 0; i < strlen(arg); i++)
	{
		if (arg[i] == '/')
		{
			strncpy(firstHalf, arg, i);
			firstHalf[i] = '\0';
			break;
		}     
	}

	for (i; i < strlen(arg); i++)
	{
		strcat(secondHalf, &arg[i]);
		break;
	}

	printf("First Half: %s\n", firstHalf);

	printf("Second Half: %s\n", secondHalf);

	int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (tcpSocket < 0)
		printf("Error opening socket\n");
	else
		printf("Successfully opened socket\n");

	server = (struct hostent *)gethostbyname(firstHalf);

	if (server == NULL)
	{
		printf("gethostbyname() failed\n");
		exit(1);
	}
	else
	{
		printf("\n%s = ", server->h_name);
		unsigned int j = 0;
		while (server -> h_addr_list[j] != NULL)
		{
			printf("%s", inet_ntoa(*(struct in_addr*)(server -> h_addr_list[j])));
			j++;
		}
	}

	printf("\n");

	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);

	serveraddr.sin_port = htons(port);

	if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		printf("Error Connecting\n");
	else
		printf("Successfully Connected\n");

	bzero(requestEnvoye, 1000);


	sprintf(requestEnvoye, "Get http://%s%s HTTP/1.1\r\nHost: %s\r\n\r\n", firstHalf, secondHalf, firstHalf);
	printf("Debug Proxy: requette HTTP effeectuée :\n");
	printf("%s\n", requestEnvoye);

	if (send(tcpSocket, requestEnvoye, strlen(requestEnvoye), MSG_CONFIRM) < 0)
        	fprintf(stderr, "Error with send()\n");
    	else
        	fprintf(stderr, "Successfully sent html fetch requestEnvoye\n");

	bzero(bufferReceive, sizeof(bufferReceive));

	// TODO recv()
	int size_recv , total_size= 0;
	char chunk[CHUNK_SIZE];
	char* html;
	html = (char*)malloc(total_size*sizeof(char));
	/* receinve
	while( 1 ){
		size_recv =  recv(tcpSocket , chunk , CHUNK_SIZE , 0);
		printf("Size receive = %d",size_recv);
		memset(chunk ,0 , CHUNK_SIZE);  //clear the variable
		if(size_recv < 0)
		{
		    printf("Debug Proxy : Waiting a bit, error %d\n",size_recv);
		    //if nothing was received then we want to wait a little before trying again, 0.1 seconds
		    usleep(100000);
		}else if (size_recv == 0){
			printf("Disconecting, %d\n", size_recv);
			break;
		}
		else
		{
		    printf("Debug Proxy : Receiving chunk %d \n",size_recv);
		    //printf("%s\n",chunk);
		    total_size += size_recv;
		    

		    strcat(html,chunk);
		}
	}*/
	// TODO end recv()

	//==
	/* receive the response */
	  //now it is time to receive the page
	char buf[CHUNK_SIZE+1];
	memset(buf, 0, sizeof(buf));
	int htmlstart = 0;
	int tmpres;
	char * htmlcontent;
	char * htmlstop;
	FILE *f = fopen("file.txt", "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}
	htmlcontent = (char*)malloc((CHUNK_SIZE+1)*sizeof(char));
	while((tmpres = recv(tcpSocket, buf, BUFSIZ, 0)) > 0 ){
		printf("tmpres : %d ================================= \n",tmpres);

		htmlcontent = buf;
		fprintf(f, "%s", htmlcontent);
		nsnd = echoToClient(sockfd, msg, nrcv);
		printf("%s",htmlcontent);
		//fprintf(stdout, htmlcontent);
		printf("check end head !\n");
		htmlstop = strstr(buf, "</HTML>");

		memset(buf, 0, tmpres+1);
		printf("is machin in : |%s|\n",htmlstop);

		if(htmlstop != NULL){
			break;
		}
	}
	if(tmpres < 0)
	{
		perror("Error receiving data");
	}
	//==

	//int nbrcv = recv(tcpSocket, bufferReceive, sizeof(bufferReceive)-1, 0);
	printf("======================================\n");
	//printf("%s\n", html);
	printf("======================================\n");

	close(tcpSocket);

	// fin ======================================================

	printf("DEBUG Proxy: FIN TEST recuperation de l'html et renvoie au client\n");

	return( nsnd );
}


void showHelp(){
	printf("HELP Proxy : Proxy <n_Port>\n");
}

int main(int argc,char *argv[]){
	int sockfd;
	int n;
	int newsockfd;
	int childpid;
	int servlen;
	int fin;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	socklen_t clilen;

	int tab_clients[FD_SETSIZE];
	fd_set rset;
	fd_set pset;

	int maxfdp1;
	int nbfd;
	int i;
	int sockcli;

	if(argc != 2){
		showHelp();
		exit(1);
	}

	// Debug d'aide
	printf("================================ WELCOME to ADBLOCK/PROXY SERVER ================================\n");
	printf("HELP Proxy : Pour se connecter en client, dans un autre terminal faire 'tellnet localhost n_Port'\n");
	printf("HELP Proxy : Pour quitter un client faire 'ctrl+altgr+] => enter => q => enter'\n");
	printf("HELP Proxy : Pour quitter le serveur (ici) faire 'ctrl+c'\n");
	printf("=================================================================================================\n");

	// Creation de la socket de dialogue
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) <0) {
		printf("ERROR Proxy: Erreur lors de la creation de la socket de dialogue !\n");
		exit (1);
	}

	// Bind
	memset( (char*) &serv_addr,0, sizeof(serv_addr) );
	serv_addr.sin_family = PF_INET;
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	if (bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr) ) <0) {
		printf("ERROR Proxy: Erreur lors du bind, attendre un peu ou utiliser un autre nPort !\n");
		exit (1);
	}

	// Listen
	if (listen(sockfd, SOMAXCONN) < 0) {
		printf("ERROR Proxy: Erreur lors du listen !\n");
		exit (1);
	}

	// Initialisation
	maxfdp1 = sockfd+1;
	for(i = 0; i < FD_SETSIZE; i++){
		tab_clients[i] = -1;
	}
	FD_ZERO(&rset);
	FD_ZERO(&pset);
	FD_SET(sockfd, &rset);

	// Boucle infinie
	for (;;) {

		pset = rset;
		// nbfd est le nombre de client qui ont fait qqch et que l'on doit traiter
		// evite de parcourir la fin du tableau dans le while plus loin
		nbfd = select(maxfdp1, &pset, NULL, NULL, NULL);

		// vrai si sockfd est dans pset
		// sockfd c'est le socket de dialogue du listen => si il est activé ya un nouveau client
		if( FD_ISSET(sockfd, &pset) ){
			// Une demande de connection a ete emise par un client
			clilen = sizeof(cli_addr);
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			// Recherche d'une place libre dans le tableau
			i = 0;
			while( (i < FD_SETSIZE) && (tab_clients[i] >= 0) ){
				i++;
			}
			if( i == FD_SETSIZE){ exit(1); }

			//Ajoute le nouveau client dans le tableau des clients
			tab_clients[i] = newsockfd;
			
			//Ajoute le nouveau client dans rset
			FD_SET(newsockfd, &rset);

			printf("DEBUG Proxy: Le client sur la socket %d c'est connecte\n",newsockfd);
	
			//Positionner maxfdp1
			if(newsockfd >= maxfdp1){
				maxfdp1 = newsockfd + 1;
			}
			nbfd--;
		}

		//Parcourir le tableau des clients connectés
		i = 0;
		while( (nbfd > 0) && (i < FD_SETSIZE) ){
			// Le client demande qqch
			if( ((sockcli = tab_clients[i]) >= 0) && (FD_ISSET(tab_clients[i], &pset)) ){
				//ici on traite les données du client
				//le client ferme sa co
				if( strRequest(sockcli) == 0 ){
					close(tab_clients[i]);
					tab_clients[i] = -1;
					FD_CLR(sockcli, &rset);
					printf("DEBUG Proxy: Le client sur la socket %d c'est deconnecte\n",sockcli);
				}
				nbfd--;
			}
			i++;
		}

		/*
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen);
		if (newsockfd < 0) {
			perror("servmulti : erreur accept\n");
			exit (1);
		}


		if ( (childpid = fork() ) < 0) {
			perror ("server: fork error\n");
			exit (1);
		}
		else
			if (childpid == 0) {       
				close (sockfd);
			while (str_echo (newsockfd));  
				exit (0); 
		}

		close(newsockfd);
		*/  
	}
}


