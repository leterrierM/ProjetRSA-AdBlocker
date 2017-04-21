	printf("DEBUG Proxy: Parsing...\n");

	// On parse avec espace et on recupe le 2 et 5 token
	char *token;
	const char delimiter[] = " \r\n";
	// On recupere split[0] dans token
	token = strtok(msg, delimiter);
	char* siteNamePath; // Token 2
	char* path; // A retrouver avec siteNamePath en parsant sur /
	char* host; // Token 5
	// Token contient split[i] successivement jusqu'au dernier ou il vaut null
	i = 0;
	while( token != NULL ){
		//printf( "%s / %d\n", token, strlen(token) );
		if(i == 1){
			siteNamePath = (char*)malloc(strlen(token)*sizeof(char));
			siteNamePath = token;
			//printf("saving token : | %s |\n",siteNamePath);
		} else if(i == 4){
			host = (char*)malloc(strlen(token)*sizeof(char));
			host = token;
			//printf("saving token : | %s |\n",host);
		}
		token = strtok(NULL, delimiter);
		i++;
	}

	// Il y a encore un peu a faire sur siteNamePath
	char *token2;
	const char delimiter2[] = "//";
	token2 = strtok(siteNamePath, delimiter2);
	token2 = strtok(siteNamePath, delimiter2);
	free(siteNamePath);
	siteNamePath = (char*)malloc(strlen(token)*sizeof(char));
	siteNamePath = token2;
	printf("saving token : | %s |\n",siteNamePath);

	// maintenant, on co
	for (i = 0; i < strlen(arg); i++)
	{
		if (arg[i] == '/')
		{
			strncpy(firstHalf, arg, i);
			firstHalf[i] = '\0';
			break;
		}     
	}

	printf("DEBUG Proxy: Parsing done !\n\n");




==================================================================================================================



	// ========================= OLD


	// TODO Peut etre faire une methode qui renvoie un char** tableau de chaine de char
	printf("DEBUG Proxy: TEST strtok\n");

	// allocate space for GET, HOST, User-Agent, Accept,Accept-Language, Accept-Encoding, Connection, Cache-Control
	// A voir si autre cookie + post par exemple
	// Plus si ya trop d'info ca fait segfault
	// en fait j'ai l'impression ya besoin de juste les 2 premiere ligne sachant que les autres sont optionnel...
	/*
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
	}*/
	
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
