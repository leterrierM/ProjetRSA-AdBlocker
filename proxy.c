#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//pour le client
#include <netdb.h>
#include <sys/select.h>

#define BUFSIZE 1500
#define MAXLINE 80
#define CHUNK_SIZE 512

#define MAXREQUESTSIZE 4096
//#define MAXHTMLSIZE 10485760 //10Mo (en moyenne html = 2Mo) // segfault si trop haut....
#define MAXHTMLSIZE 5242880 //5Mo
int i;

/* ============================================ HANDLECLIENT ============================================ */
int handleClient(int sockfd){
	// Si le client se deconnecte, on renvoit 0

	// contient le nb d'octet lu => pointe juste apres le dernier caractere dans msg on peut donc y mettre \0
	int nrcv;
	int nsnd;
	char clientRequest[MAXREQUESTSIZE];

	printf("DEBUG Proxy: Debut du traitement ================================\n");



	//On lit le msg du client et on le range dans clientRequest
	printf("DEBUG Proxy: Debut de recuperation de la requette du client\n");
	memset( (char*) clientRequest, 0, sizeof(clientRequest) );
	if ( (nrcv = read ( sockfd, clientRequest, sizeof(clientRequest)-1) ) < 0 )  {
		printf("ERROR Proxy: Erreur lors de la lecture sur le socket %d !\n",sockfd);
		exit (1);
	}
	clientRequest[nrcv]='\0';

	printf("DEBUG Proxy: Message de taille %d recu sur le socket %d du processus %d  !\n",nrcv,sockfd,getpid());
	printf("DEBUG Proxy: Debut du message de la socket %d :\n",sockfd);
	printf("%s",clientRequest);
	printf("DEBUG Proxy: Fin du message === \n\n");



	printf("DEBUG Proxy: Sauvegarde du message dans un fichier\n");
	char fileName[32];
	sprintf(fileName,"TMP_clientRequest_%d.txt",sockfd);
	FILE *frequest = fopen(fileName, "w");
	int fdRequest = fileno(frequest);
	if (frequest == NULL)
	{
	    printf("ERROR Proxy: Erreur lors de l'ouverture de %s!\n",fileName);
	    exit(1);
	}
	if ( (nsnd = write (fdRequest, clientRequest, nrcv) ) < 0 ) {
		printf("%d/%d\n",nsnd,nrcv);
		printf("ERROR Proxy: Erreur lors de l'ecriture dans %s!\n",fileName);
		exit (1);
	}
	fflush(frequest);
	if ( fclose(frequest) ) {
		printf("ERROR Proxy: Erreur lors de la fermeture de %s!\n",fileName);
		exit (1);
	}
	frequest = NULL;
	printf("DEBUG Proxy: Sauvegarde terminée\n\n");
	//printf("DEBUG Proxy: Fin de recuperation de la requette du client\n\n");



	// on retourne direct si le client se deco 
	if(nsnd == 0){
		printf("DEBUG Proxy: Deconexion du client! (nsnd : %d)\n",nsnd);
		return nsnd;
	}

	// Si c'est un connect on quitte (comme pour un deco)
	char* isGet = strstr(clientRequest,"CONNECT");
	if(isGet != NULL){
		printf("DEBUG Warning: CONNECT n'est pas implementé! (isGet : %s)\n",isGet);
		return 0;
	}



	printf("DEBUG Proxy: Parsing to get host...\n");
	char *token;
	const char delimiter[] = " \r\n";
	char* host;
	// on copie la requette sinon elle est modifiee par strtok
	char clientRequestCopy[MAXREQUESTSIZE];
	strcpy(clientRequestCopy,clientRequest);
	// On recupere split[0] dans token
	token = strtok(clientRequestCopy, delimiter);

	i = 0;
	// Token contient split[i] successivement jusqu'au dernier ou il vaut null
	while( strcmp(token, "Host:") != 0){
		//printf( "%s / %d\n", token, strlen(token) );
		token = strtok(NULL, delimiter);
		i++;
	}
	token = strtok(NULL, delimiter);
	host = (char*)malloc(strlen(token)*sizeof(char));
	host = token;
	
	printf("DEBUG Proxy: Parsing (%s) OK!\n\n",host);

	

	int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent* hostServeur;
	struct sockaddr_in serveraddr;

	printf("DEBUG Proxy: Creation de la socket TCP...\n");
	if (tcpSocket < 0){
		printf("ERROR Proxy: Erreur lors de la creation de la socket TCP!\n");
		exit (1);
	}else{
		printf("DEBUG Proxy: Creation de la socket TCP : OK!\n");
	}

	printf("DEBUG Proxy: Recherche de %s...\n",host);
	char hostConst[500];
	strcpy(hostConst, host);
	hostServeur = (struct hostent *)gethostbyname(host);
	if (hostServeur == NULL)
	{
		printf("ERROR Proxy: Erreur lors de la recherche de %s! (Pas d'internet?)\n", host);
		exit(1);
	}else{
		printf("DEBUG Proxy: Connexion a %s : OK!\n",host);
		printf("------------------------\n");
		printf("%s = ", hostServeur->h_name);
		unsigned int j = 0;
		while (hostServeur -> h_addr_list[j] != NULL)
		{
			printf("%s\n", inet_ntoa(*(struct in_addr*)(hostServeur -> h_addr_list[j])));
			j++;
		}
		printf("------------------------\n");
	}

	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;

	bcopy((char *)hostServeur->h_addr, (char *)&serveraddr.sin_addr.s_addr, hostServeur->h_length);

	serveraddr.sin_port = htons(80);
	
	printf("DEBUG Proxy: Connexion...\n");
	if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
		printf("ERROR Proxy: Erreur lors de la conexion a %s!\n", host);
		exit(1);
	}else{
		printf("DEBUG Proxy: Connexion : OK!\n");
	}
	
	printf("REQUETE :\n");
	printf("------------------------\n");
	printf("%s\n\n%d\n", clientRequest, (int)strlen(clientRequest));
	printf("------------------------\n");
	printf("DEBUG Proxy: Envoi de la requete HTTP...\n");
	if (send(tcpSocket, clientRequest, strlen(clientRequest), MSG_CONFIRM) < 0){
		printf("ERROR Proxy: Erreur lors de l'envoi de la requete HTTP !\n");
		exit(1);
	}else{
		printf("DEBUG Proxy: Envoi de la requete HTTP : OK!\n");
	}


	//TODO recv ====================================
	printf("DEBUG Proxy: Reception du contenu HTML!\n");
	char fileNameHTML[32];
	sprintf(fileNameHTML,"TMP_HTML_%d.txt",sockfd);
	FILE *fhtml = fopen(fileNameHTML, "w");
	int fdhtml = fileno(fhtml);
	int htmlsize;
	int htmlsize2;
	char reponseHTML[MAXHTMLSIZE];
	
	// Reception de la page
	htmlsize = recv(tcpSocket, reponseHTML, MAXHTMLSIZE, 0);
	// ecritur de la page dans un fichier tmp
	if (fhtml == NULL){
	    printf("ERROR Proxy: Erreur lors de l'ouverture de %s!\n",fileNameHTML);
	    exit(1);
	}
	if ( (htmlsize2 = write (fdhtml, reponseHTML, htmlsize) ) < 0 ) {
		printf("%d/%d\n",htmlsize,htmlsize2);
		printf("ERROR Proxy: Erreur lors de l'ecriture dans %s!\n",fileNameHTML);
		exit (1);
	}
	fflush(fhtml);
	if ( fclose(fhtml) ){
		printf("ERROR Proxy: Erreur lors de la fermeture de %s!\n",fileNameHTML);
		exit (1);
	}
	fhtml = NULL;
	
	if(htmlsize < 0){
		perror("Error receiving data");
		exit(0);
	}
	
	//fermeture de la co avec l'host
	close(tcpSocket);
	printf("DEBUG Proxy: Fin reception du contenu HTML!\n\n");

	printf("DEBUG Proxy: Reponse HTML:\n");
	printf("------------------------\n");
	printf("%s\n",reponseHTML);
	printf("------------------------\n");

	// renvoie de la page auclient
	printf("DEBUG Proxy: Envoi du contenu HTML!\n");
	
	int htmlwrite;
	if ( (htmlwrite = write (sockfd, reponseHTML, htmlsize) ) <0 ) {
		printf("ERROR Proxy: Erreur lors de l'ecriture sur le socket %d !\n",sockfd);
		exit (1);
	}

	printf("DEBUG Proxy: Fin envoi du contenu HTML!\n\n");

	printf("DEBUG Proxy: Fin du traitement! ================================\n\n\n");

	return nsnd;
}

/* ================================================ HELP ================================================ */
void showHelp(){
	printf("HELP Proxy : Proxy <n_Port>\n");
}

/* ================================================ MAIN ================================================ */
int main(int argc,char *argv[]){
	// Variable pour la partie SELECT() du proxy
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
				//dans handleClient ou on fait proxy -> server -> proxy -> client
				if( handleClient(sockcli) == 0 ){
					close(tab_clients[i]);
					tab_clients[i] = -1;
					FD_CLR(sockcli, &rset);
					printf("DEBUG Proxy: Le client sur la socket %d c'est deconnecte\n",sockcli);
				}
				nbfd--;
			}
			i++;
		}
	}
}
