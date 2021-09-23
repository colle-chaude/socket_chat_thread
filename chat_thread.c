/* programme (c) M Kerrisk,
   adapté par P. Lalevée */

#include <stdio.h>
#include <stdarg.h>
#include "pse.h"
#include <regex.h>

#define BUF_SIZE 1024
#define PRINT_LINE_MAX LIGNE_MAX + 70
//#define    CMD      "serveur"


typedef struct
{
  char line[PRINT_LINE_MAX];
  int new_line;
  int flag;
}to_print_struct;


typedef struct
{
  char addr_str[30];
  char port_str[8];

  to_print_struct print;
  to_print_struct log;

  struct sockaddr_in *adrServ;
}cli_struct;

typedef struct 
{
  char port_str[8];
  char pseudo_distant[30];


  to_print_struct print;
  to_print_struct log;

  short listen_port;
}serv_struct;



void* thread_cli_f(void* arg);
void* thread_serv_f(void* arg);
int print_line(to_print_struct* __toprint, const char *__restrict__ __format, ...);



  to_print_struct* list_print[30];
  to_print_struct* list_log[30];
  int nb_print = 0;
  int nb_log = 0;

  char pseudo_local[30] =   "local--";
  char log_file_str[60] = "journal.log";
  int log_file;


int main(int argc, char *argv[]) {

  int run = 1; 

  serv_struct serveur_s = {"2000", "distant"};
  cli_struct client_s = {"localhost", "2000"};

  list_log[nb_log++] = &serveur_s.log;
  list_log[nb_log++] = &client_s.log;

  list_print[nb_print++] = &serveur_s.print;
  list_print[nb_print++] = &client_s.print;


  for(int i = 0; i< nb_log; i++)
  {
    list_log[i]->flag = 1;
    list_log[i]->new_line = 0;
  }
  for(int i = 0; i< nb_print; i++)
  {
    list_print[i]->flag = 1;
    list_print[i]->new_line = 0;
  }


  regex_t  arg_ex;
  regmatch_t arg_match[2];
  regcomp(&arg_ex, "^-(\\w)$", REG_EXTENDED|REG_NOSUB);//regex qui cherche les -x (x lettre associée au paramètre)
  for(int i = 1; i<argc; i++)
  {
    if( regexec(&arg_ex, argv[i], 1, arg_match, 0) == 0)
    {
     
      switch (argv[i][1])
      {
      case 'a':
        if(i < argc-1)strcpy(client_s.addr_str, argv[i+1]);
        break;
      case 'p':
        if(i < argc-1)strcpy(client_s.port_str, argv[i+1]);
        break;
      case 'l':
        if(i < argc-1)strcpy(serveur_s.port_str, argv[i+1]);
        break;
      case 'u':
        if(i < argc-1)strcpy(pseudo_local, argv[i+1]);
        break;
      case 'o':
        if(i < argc-1)strcpy(log_file_str, argv[i+1]);
        break;
      case 'i':
        if(i < argc-1)strcpy(serveur_s.pseudo_distant, argv[i+1]);
        break;
      case 'h':
        printf("%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s","Help : ",
        "-a : addresse de l'interlocuteur",
        "-p : port de l'interlocuteur",
        "-l : port d'ecoute local",
        "-u : mon username",
        "-o : fichier de log",
        "-i : username de l'interlocuteur");
        run = 0;
        /* code */
        break;
      
      default:
        break;
      }
    }
  }


if(run)
{



  log_file = open(log_file_str,O_CREAT|O_WRONLY|O_APPEND, 0644);
  if (log_file == -1) {
    char errmessage[90];
    sprintf(errmessage, "open output, unable to open \"%s\"", log_file_str);
    erreur_IO(errmessage);
  }


  pthread_t pthread_cli, pthread_serv;

  printf("creating thread\n");
  int ret_cli = pthread_create(&pthread_cli, NULL, &thread_cli_f, &client_s);
  int ret_serv = pthread_create(&pthread_serv, NULL, &thread_serv_f, &serveur_s);
  printf("thread created\n");
  while(run)
  {  
    for(int i = 0; i < nb_print; i++)
    {
      if(list_print[i]->flag && list_print[i]->new_line)
      {
        printf(list_print[i]->line);
        fflush(stdout);
        list_print[i]->new_line = 0;
      }
    }  
    for(int i = 0; i < nb_log; i++)
    {
      if(list_log[i]->flag && list_log[i]->new_line)
      {
        printf(list_log[i]->line);
        fflush(stdout);
        list_log[i]->new_line = 0;
      }
    }  
    
  }

}
}

void* thread_cli_f(void* arg)
{
  cli_struct* cli = (cli_struct*) arg;
  char CMD[] ="\rclient";
  print_line(&cli->print,"%s: thread cli start\n", CMD);
  
  signal(SIGPIPE, SIG_IGN);
  //int inputFd, log_file;
  ssize_t nbRead, nbRead_tot = 0;
 // char buf[BUF_SIZE];
  int run = 1;

  int sock, ret;
  //struct sockaddr_in *adrServ;
  char ligne[LIGNE_MAX];
  int lgEcr;




  print_line(&cli->print,"%s: creating a socket\n", CMD);
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    erreur_IO("socket");

  print_line(&cli->print,"%s: DNS resolving for %s, port %s\n", CMD, cli->addr_str, cli->port_str);
  cli->adrServ = resolv(cli->addr_str, cli->port_str);
  if (cli->adrServ == NULL)
    erreur("adresse %s port %s inconnus\n", cli->addr_str, cli->port_str);

  print_line(&cli->print,"%s: adr %s, port %hu\n", CMD,
	       stringIP(ntohl(cli->adrServ->sin_addr.s_addr)),
	       ntohs(cli->adrServ->sin_port));

  print_line(&cli->print,"%s: connecting the socket ...", CMD);
  ret = 0;
  while(connect(sock, (struct sockaddr *)cli->adrServ, sizeof(struct sockaddr_in)) < 0);
  if (ret < 0)
    erreur_IO("connect");
  print_line(&cli->print,".. connecte\n");


  char fin_ex_str[] = "^fin\n?$";
  regex_t fin_ex;
  regcomp(&fin_ex, fin_ex_str, REG_EXTENDED|REG_NOSUB);


while(run)
{
  print_line(&cli->print,"<%s>:", pseudo_local);
  //fflush(stdout);
  if (fgets(ligne, LIGNE_MAX, stdin) == NULL)
    print_line(&cli->print,"arret par CTRL-D\n");

  else {
    lgEcr = ecrireLigne(sock, ligne);
    if (lgEcr == -1)
      erreur_IO("ecrireLigne");
  
  //  print_line(cli->line_to_print,"%s: %d bytes sent\n", CMD, lgEcr);
  nbRead = strlen(ligne); 
  nbRead_tot += nbRead;
  int res_ex = regexec(&fin_ex, ligne, 0, NULL, 0);
  //print_line(cli->line_to_print,"res ex :%s: %d\n",string, res_ex);
  if( res_ex == 0)
  {
    run = 0;

    print_line(&cli->print,"%s: End sended, close serveur\n", CMD);
  }
  char output[65+LIGNE_MAX];
  int nbOutput = sprintf(output,"<%s>:%s", pseudo_local, ligne);
  int nb_write = nbOutput;//write(log_file, output, nbOutput);
      //printf(output);
      if (nb_write != nbOutput) {
        erreur_IO("write");
      }

  }


  if (nbRead == -1) {
    erreur_IO("read");
  }

}

  if (close(sock) == -1)
    erreur_IO("close");
  //if (close(log_file) == -1) {
   // erreur_IO("close output");
 // }
  print_line(&cli->print,"%s: %ld char have been transfered\n",CMD, nbRead_tot);
  return NULL;
}

void* thread_serv_f(void* arg)
{
  serv_struct* serv = (serv_struct*) arg;
  char CMD[] ="\rserveur";
  print_line(&serv->print,"%s: thread serv start\n", CMD);
  
  
  ssize_t lgLue, lgLue_tot = 0;
  //char buf[BUF_SIZE];
  int run = 1;


  int ecoute, canal, ret;
  struct sockaddr_in adrEcoute, adrClient;
  unsigned int lgAdrClient;
  char ligne[LIGNE_MAX];
  


  serv->listen_port = (short)atoi(serv->port_str);

  print_line(&serv->print,"%s: creating a socket\n", CMD);
  ecoute = socket (AF_INET, SOCK_STREAM, 0);
  if (ecoute < 0)
    erreur_IO("socket");
  
  adrEcoute.sin_family = AF_INET;
  adrEcoute.sin_addr.s_addr = INADDR_ANY;
  adrEcoute.sin_port = htons(serv->listen_port);
  print_line(&serv->print,"%s: binding to INADDR_ANY address on port %d\n", CMD, serv->listen_port);
  ret = bind (ecoute,  (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
  if (ret < 0)
    erreur_IO("bind");
  
  print_line(&serv->print,"%s: listening to socket\n", CMD);
  ret = listen (ecoute, 5);
  if (ret < 0)
    erreur_IO("listen");
  
  print_line(&serv->print,"%s: accepting a connection\n", CMD);
  lgAdrClient = sizeof(adrClient);
  canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
  if (canal < 0)
    erreur_IO("accept");

  print_line(&serv->print,"%s: adr %s, listen_port %hu\n", CMD,
         stringIP(ntohl(adrClient.sin_addr.s_addr)),
         ntohs(adrClient.sin_port));






  char fin_ex_str[] = "^fin\n?$";
  char init_ex_str[] = "^init\n?$";
  regex_t fin_ex;
  regex_t init_ex;
  regcomp(&init_ex, init_ex_str, REG_EXTENDED|REG_NOSUB);
  regcomp(&fin_ex, fin_ex_str, REG_EXTENDED|REG_NOSUB);

  while (run)
  {
    lgLue = lireLigne(canal, ligne);
    
    if(lgLue > 0)
    {
      lgLue_tot+= lgLue;
      char output[LIGNE_MAX + 65];
     // print_line(&serv->print,"%s: reception %ld octets : \"%s\"\n", CMD, lgLue, ligne);
      
      if( regexec(&fin_ex, ligne, 0, NULL, 0) == 0)
      {
        run = 0;
        print_line(&serv->print,"%s: End recieved, close serveur\n", CMD);
      }
      if( regexec(&init_ex, ligne, 0, NULL, 0) == 0)
      {
        print_line(&serv->print,"\nclean\n");
      }
      if(ligne[lgLue-1] == '\0')
      {
        ligne[lgLue-1] = '\n';
        ligne[lgLue] = '\0';
      }
      int nbOutput = sprintf(output,"<%s>:%s", serv->pseudo_distant, ligne);
      print_line(&serv->print,"\r%s",output);
      //print_line(serv->line_to_print,"  ");
      print_line(&serv->print,"<%s>:", pseudo_local);
      int nb_write = nbOutput;//write(log_file, output, nbOutput);
      if (nb_write != nbOutput) {
        erreur_IO("write");
      }
      //print_line(&serv->print,"%s: %ld char have been transfered\n",CMD, lgLue_tot);

    }
    else if (lgLue < 0)
    {
      erreur_IO("lireLigne");
    }
    else
    {
    }
  }


  
  if (close(canal) == -1)
    erreur_IO("close canal");

  if (close(ecoute) == -1)
    erreur_IO("close ecoute");

  if (close(log_file) == -1) {
    erreur_IO("close output");
  }
  

  
  //////////////////
  
  if (lgLue == -1) {
    erreur_IO("read");
  }
  print_line(&serv->print,"%s: %ld char have been transfered\n",CMD, lgLue_tot);
  return NULL;
}

int print_line(to_print_struct* __toprint, const char *__restrict__ __format, ...)
{
  int done = -1;
  if(__toprint->flag)
  {
    while(__toprint->new_line);
    va_list args;
    va_start(args, __format);
    done = vsprintf(__toprint->line, __format, args);
    va_end(args);
    __toprint->new_line = 1;
  }
  return done;
}