#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <errno.h>

//pour le client
#include <netdb.h>
#include <sys/select.h>

#define MAXREQUESTSIZE 4096

/* ================================================ HELP ================================================ */
void showHelp(){
	printf("HELP Proxy : Proxy <n_Port> [nom de fichier contenant une liste de mot clef a bloquer]\n");
}

char adlistFileName[50] = "adList";
char logFileName[10] = "logs";
char responseFileName[8] = "response";

char * adRepContent;

/* ============================================= verify_host ============================================ */
int verify_host(char* host)
{
	char *token;
	char *badHosts;
	long sizeFile;
	FILE *fdFile = fopen(adlistFileName, "r");
	fseek(fdFile, 0, SEEK_END);
	sizeFile = ftell(fdFile);
	rewind(fdFile);
	badHosts = malloc(sizeFile * (sizeof(char)));
	fread(badHosts, sizeof(char), sizeFile, fdFile);
	fclose(fdFile);

	token = strtok(badHosts,"\r\n");

	while( (token = strtok(NULL,"\r\n")) != NULL){
		if(strstr(host, token) != NULL){
			printf("DEBUG Proxy: Request contains Token (%s)\n",token);
			return 1;
		}
	}
	return 0;
}

/* ================================================ MAIN ================================================ */
int main(int argc,char *argv[]){
	
	int sockfd;
	int newsockfd;

	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;

	socklen_t clilen;

	// DEBUG d'aide
	printf("================================ WELCOME to ADBLOCK/PROXY SERVER ================================\n");
	printf("HELP Proxy : Pour se connecter en client, dans un autre terminal faire 'tellnet localhost n_Port'\n");
	printf("HELP Proxy : Pour quitter un client faire 'ctrl+altgr+] => enter => q => enter'\n");
	printf("HELP Proxy : Pour quitter le serveur (ici) faire 'ctrl+c'\n");
	printf("=================================================================================================\n\n");

	// Il faut seulement un numero de port vers lequel ecouter
	if(argc == 2 ){
		printf("DEBUG Proxy: Log output into 'logs' file\n");
		printf("DEBUG Proxy: Blocked ad into 'adList' file\n");
	}else if(argc == 3){
		printf("DEBUG Proxy: Log output into 'logs' file\n");
		strcpy(adlistFileName, argv[2]);
		printf("DEBUG Proxy: Blocked ad into %s file\n", adlistFileName);
		printf("WARNING Proxy: First line of the file is ignored !\n");
		
	}else{
		showHelp();
		exit(1);
	}

	FILE *fdrep = fopen(responseFileName, "r");
	fseek(fdrep, 0, SEEK_END);
	int repSize = ftell(fdrep);
	rewind(fdrep);
	adRepContent = malloc(repSize * (sizeof(char)));
	fread(adRepContent, sizeof(char), repSize, fdrep);
	fclose(fdrep);

	printf("\n");

	// Creation de la socket de dialogue entre le navigateur et le proxy
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) <0) {
		printf("ERROR Proxy: Erreur lors de la creation de la socket de dialogue !\n%s\n", strerror(errno));
		exit (1);
	}

	// Bind
	memset( (char*) &serv_addr,0, sizeof(serv_addr) );

	serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(atoi(argv[1]));
        serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr) ) <0) {
		printf("ERROR Proxy: Erreur lors du bind, attendre un peu ou utiliser un autre nPort !\n%s\n", strerror(errno));
		exit (1);
	}

	// Listen
	if (listen(sockfd, SOMAXCONN) < 0) {
		printf("ERROR Proxy: Erreur lors du listen !\n%s\n", strerror(errno));
		exit (1);
	}

	bzero((char * ) & cli_addr, sizeof(cli_addr));

	const char delimiter_Retour[] = "\r\n";
	const char delimiter_EspaceRetour[] = " \r\n";

	char html[500];
	char buffer[50];
	char requestToSend[MAXREQUESTSIZE];
	char clientRequest[MAXREQUESTSIZE];
	char* isConnect;
	char* isPost;
	int nborcvd;
	int nbOctetSend;
	int sockfd1;
	int newsockfd1;
	char *token;
	char* cookie ;
	char* host;
	char* get;
	char* postLength;
	char* postContent;
	char clientRequestCopy[MAXREQUESTSIZE];
	struct sockaddr_in host_addr;
	struct hostent * hostStruct;

	while(1)
	{
		clilen = sizeof(cli_addr);

		printf("DEBUG Proxy: Waiting for the Accept\n");
		newsockfd = accept(sockfd, (struct sockaddr * ) & cli_addr, & clilen);
		printf("DEBUG Proxy: Accepting a request on newsockfd : %d\n",newsockfd);
		// Fork : traitement par le fils de la requette demandée (le pere attend de nouveau une connexion entrante)
		if(fork() == 0)
		{
			bzero((char * ) & clientRequest, sizeof(clientRequest));

			nborcvd = recv(newsockfd, clientRequest, sizeof(clientRequest), 0);
			
			// On accepte pas les CONNECTs
			isConnect = strstr(clientRequest,"CONNECT ");
			if(isConnect != NULL){
				printf("DEBUG Warning: CONNECT n'est pas implementé! (isGet : %s)\n",isConnect);
				close(newsockfd);
				exit(2);
			}
			else if(nborcvd < 0)
			{
				printf("DEBUG Warning: Requete non coherente\n");
				close(newsockfd);
				exit(3);
			}else if(nborcvd == 0)
			{
				printf("DEBUG Warning: Deconnexion du client %d\n",newsockfd);
				close(newsockfd);
				exit(0);
			}
			else
			{
				printf("DEBUG Proxy: Affichage de la Requette :\n");
				printf("%s\n",clientRequest);
				printf("=======================================\n\n");

				// Analyse de la requette pour recuperer le host, les cookies, et la premiere ligne (GET/POST/CONNECT)
				printf("DEBUG Proxy: Parsing to get host...\n");

				// Copie la requette, sinon elle est modifiee par strtok
				strcpy(clientRequestCopy,clientRequest);
				// On recupere split[0] dans token, on split avec " \r\n" => il y a deux delimiteurs
				token = strtok(clientRequestCopy, delimiter_EspaceRetour);

				// Token contient split[i] successivement jusqu'au dernier, ou il vaut NULL
				// On cherche le token qui vaut "Host: "
				while( strcmp(token, "Host:") != 0){
					//printf( "%s / %d\n", token, strlen(token) );
					token = strtok(NULL, delimiter_EspaceRetour);
				}
				// Lorsque on le trouve on sait que le token suivant est la valeur du host
				token = strtok(NULL, delimiter_EspaceRetour);
				host = (char*)malloc(strlen(token)*sizeof(char));
				strcpy(host,token);
				// Reset de la fonction strtok
				while( strtok(NULL, delimiter_EspaceRetour) != NULL){
				}
				printf("DEBUG Proxy: Parsing host : (%s)\n\n",host);

				// Copie
				strcpy(clientRequestCopy,clientRequest);

				printf("DEBUG Proxy: Parsing to get (GET,POST,CONNECT path HTTP/1.1)...\n");
				// La premiere ligne 
				token = strtok(clientRequestCopy, delimiter_Retour);
	
				get = (char*)malloc(strlen(token)*sizeof(char));
				strcpy(get,token);
				printf("DEBUG Proxy: Parsing get (%s)\n\n",get);

				printf("DEBUG Proxy: Parsing to get cookie...\n");
				while(token !=NULL){
					
					token = strtok(NULL, delimiter_Retour);
					if(token !=NULL && strstr(token, "Cookie: ") != NULL)
					{
						if(cookie != NULL)
							free(cookie);
						cookie = (char*)malloc(strlen(token)*sizeof(char));
						strcpy(cookie,token);
					}
				}
				printf("DEBUG Proxy: Parsing Cookie (%s)\n\n",cookie);

				isPost = strstr(clientRequest,"POST ");
				if(isPost != NULL){
					printf("DEBUG Proxy: POST requette detectée ! Recuperation du contenu et de sa taille...\n");
					// Copie
					strcpy(clientRequestCopy,clientRequest);

					// La derniere ligne 
					token = strtok(clientRequestCopy, delimiter_Retour);
	
					printf("DEBUG Proxy: Parsing to get cookie...\n");
					while(token !=NULL){
						postContent = (char*)malloc(strlen(token)*sizeof(char));
						strcpy(postContent,token);
						token = strtok(NULL, delimiter_Retour);
						if(token !=NULL && strstr(token, "Content-Length: ") != NULL)
						{
							if(cookie != NULL)
								free(postLength);
							postLength = (char*)malloc(strlen(token)*sizeof(char));
							strcpy(postLength,token);
						}
					}
					printf("DEBUG Proxy: POST requette detectée ! Recuperation du %s et de sa %s...\n",postContent,postLength);
				}


				

				bzero((char * ) & clientRequestCopy, sizeof(clientRequestCopy));


				// Verification si la requette pourrait contenir une pub
				// On compare l'hote et le chemin de la requette a une base de données en fichier texte
				if(verify_host(get) == 1 || verify_host(host) == 1)
				{
					printf("DEBUG Proxy: PUB detectée!\n");
					
					printf("DEBUG Proxy: Sauvegarde de la requette publicitaire !\n\n");

					time_t mytime;
					mytime = time(NULL);

					FILE *fLogFile = fopen(logFileName, "a");
					
					//printf("DEBUG Proxy: opening %s!\n",fileName);
					if (fLogFile == NULL){
						printf("ERROR Proxy: Erreur lors de l'ouverture de logs.log!\n%s\n", strerror(errno));
						exit (1);
					}

					fprintf(fLogFile, "===================================================\nDATE : %shost : %s\n===\nget : %s\n",ctime(&mytime), host, get);

					if ( fclose(fLogFile) ){
						printf("ERROR Proxy: Erreur lors de la fermeture de logs.log!\n%s\n", strerror(errno));
						exit (1);
					}
					fLogFile = NULL;

					send(newsockfd, adRepContent, sizeof(adRepContent), 0);

				}
				else
				{
					//Connection a l'host
					printf("DEBUG Proxy: Connecting to host : %s !\n\n", host);
					hostStruct = gethostbyname(host);
					if (hostStruct == NULL)
					{
						printf("ERROR Proxy: Erreur lors de la recherche de %s! (Pas d'internet?)\n%s\n", host, strerror(errno));
						exit (1);
					}
					else{
						bzero((char * ) & host_addr, sizeof(host_addr));
				
						host_addr.sin_family = AF_INET;
						host_addr.sin_port = htons(80);
						
						//printf("copying : %s\n",(char * ) hostStruct -> h_addr);
						
						memcpy(  (char * ) & host_addr.sin_addr.s_addr, (char * ) hostStruct -> h_addr, hostStruct -> h_length);
	
						// Creation de la socket de dialogue le proxy et l'internet
						printf("DEBUG Proxy: Creation de la socket de dialogue entre le proxy et %s!\n",host);
						if ((sockfd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ) <0) {
							printf("ERROR Proxy: Erreur lors de la creation de la socket !\n%s\n", strerror(errno));
							exit (1);
						}
						printf("DEBUG Proxy: Connection a %s!\n",host);
						newsockfd1 = connect(sockfd1, (struct sockaddr * ) & host_addr, sizeof(struct sockaddr));
						if (newsockfd1 < 0){
							printf("ERROR Proxy: Erreur lors de la connection au serveur distant\n%s\n", strerror(errno));
							exit (1);
						}
						// Affiche l'ip du host
				
						sprintf(buffer, "Connecte a %s  IP - %s\n", host, inet_ntoa(host_addr.sin_addr));
						printf("\n%s\n", buffer);

						// Creation de la requette a envoyer
				
						printf("DEBUG Proxy: Simplification de la requette :\n");
						bzero((char * ) requestToSend, sizeof(requestToSend));

						if(isPost == NULL){
							if(cookie != NULL){
								sprintf(requestToSend, "%s\r\nHost: %s\r\n%s\r\nUser-Agent: Proxy\r\nConnection: close\r\n\r\n", get, host,cookie);
							}else{
								sprintf(requestToSend, "%s\r\nHost: %s\r\nUser-Agent: Proxy\r\nConnection: close\r\n\r\n", get, host);
							}
						}else{
							if(cookie != NULL){
								sprintf(requestToSend, "%s\r\nHost: %s\r\n%s\r\nUser-Agent: Proxy\r\nConnection: close\r\n%s\r\n\r\n%s\r\n", get, host, cookie, postLength, postContent);
							}else{
								sprintf(requestToSend, "%s\r\nHost: %s\r\nUser-Agent: Proxy\r\nConnection: close\r\n%s\r\n\r\n%s\r\n", get, host, postLength, postContent);
							}
						}
						printf("%s", requestToSend);
						printf("=======================================\n\n");

						printf("DEBUG Proxy: Envoi de la requette!\n");
						nbOctetSend = send(sockfd1, requestToSend, strlen(requestToSend), 0);

						bzero((char * ) html, sizeof(html));
				
						if (nbOctetSend < 0){
							printf("ERROR Proxy: Erreur lors de l'envoi de la requette!\n%s\n", strerror(errno));
							exit (1);
						}
						else {
							printf("DEBUG Proxy: Debut de Transmission\n");
							do {
								bzero((char * ) html, sizeof(html));
								
								nbOctetSend = recv(sockfd1, html, sizeof(html), 0);
								printf("DEBUG Proxy: Envoi de %d Octets\n", nbOctetSend);
								if ( !(nbOctetSend <= 0) )
									send(newsockfd, html, nbOctetSend, 0);
							} while (nbOctetSend > 0);
							printf("DEBUG Proxy: Fin de Transmission");
						}
						
						close(sockfd1);
						close(newsockfd1);
					}
				}
				free(get);
				free(host);
			}
			close(newsockfd);
			_exit(0);
		} else { // fork() != 0 => pere
			close(newsockfd);
		}
	} // END WHILE
	return 0;
}
